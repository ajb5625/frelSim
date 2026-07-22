# ===== Config =====
CXX        := g++
PROTOC     := protoc

# 0 = static (.a), 1 = shared (.so)
BUILD_SHARED ?= 0
DEBUG        ?= 1

# Project layout
SRC_DIRS    := frelsim
PROTO_DIR   := frelsim/proto
OBJ_DIR     := build
BIN_DIR     := build
LIB_NAME    := frelsim
EIGEN_INC   := /usr/include/eigen3

# ===== Protobuf flags (prefer pkg-config, fallback) =====
PROTOBUF_CFLAGS := $(shell pkg-config --cflags protobuf 2>/dev/null)
PROTOBUF_LIBS   := $(shell pkg-config --libs protobuf 2>/dev/null)
ifeq ($(PROTOBUF_CFLAGS),)
  PROTOBUF_CFLAGS := -I/usr/include
endif
ifeq ($(PROTOBUF_LIBS),)
  PROTOBUF_LIBS := -lprotobuf
endif

# ===== gRPC detection =====
# There may be more than one grpc install on a machine (e.g. a system package
# alongside a locally-built one). protoc's generated code must match whichever
# grpc++ headers/libs it's compiled against, so prefer the system plugin
# (matching the system libgrpc++-dev/protobuf this project links against)
# over whatever else might be earlier on PATH, and derive the pkg-config
# search path from wherever it actually resolves.
GRPC_CPP_PLUGIN := $(if $(wildcard /usr/bin/grpc_cpp_plugin),/usr/bin/grpc_cpp_plugin,$(shell which grpc_cpp_plugin 2>/dev/null))
GRPC_PREFIX     := $(patsubst %/bin/grpc_cpp_plugin,%,$(GRPC_CPP_PLUGIN))
ifneq ($(strip $(GRPC_PREFIX)),)
  GRPC_PKG_CONFIG_PATH := $(GRPC_PREFIX)/lib/pkgconfig:$(GRPC_PREFIX)/lib64/pkgconfig
endif
GRPC_CFLAGS := $(shell PKG_CONFIG_PATH="$(GRPC_PKG_CONFIG_PATH):$$PKG_CONFIG_PATH" pkg-config --cflags grpc++ 2>/dev/null)
GRPC_LIBS   := $(shell PKG_CONFIG_PATH="$(GRPC_PKG_CONFIG_PATH):$$PKG_CONFIG_PATH" pkg-config --libs grpc++ 2>/dev/null)
ifeq ($(GRPC_CFLAGS),)
  GRPC_CFLAGS := -I/usr/include
endif
ifeq ($(GRPC_LIBS),)
  GRPC_LIBS := -lgrpc++ -lgrpc -lpthread
endif
ifeq ($(GRPC_CPP_PLUGIN),)
  $(warning grpc_cpp_plugin not found in PATH; gRPC stubs may not be generated)
endif

# ===== Flags =====
WARN     := -Wall -Wextra -Wconversion -Wshadow
STD      := -std=c++20
ifeq ($(DEBUG),1)
  OPT   := -O0 -g3
else
  OPT   := -O3 -g0
endif

CXXFLAGS := $(STD) $(WARN) $(OPT) -MMD -MP -Iinclude -I$(OBJ_DIR) -I$(EIGEN_INC) $(PROTOBUF_CFLAGS) $(GRPC_CFLAGS)
LDFLAGS  :=
LDLIBS   := $(PROTOBUF_LIBS) $(GRPC_LIBS)

ifeq ($(BUILD_SHARED),1)
  CXXFLAGS += -fPIC
  TARGET    := $(BIN_DIR)/lib$(LIB_NAME).so
  LDFLAGS   += -shared
else
  TARGET    := $(BIN_DIR)/lib$(LIB_NAME).a
endif

# ===== Sources =====
# Regular C++ sources (ignore generated pb.cc)
SRC_CPP  := $(shell find $(SRC_DIRS) -type f -name '*.cpp' ! -name '*.pb.cpp' ! -name '*.pb.cc' 2>/dev/null)
OBJ_CPP  := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC_CPP))

# Proto sources
PROTO_SRC := $(wildcard $(PROTO_DIR)/*.proto)

# Generated message sources/headers
PROTO_GEN_CC  := $(patsubst $(PROTO_DIR)/%.proto,$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc,$(PROTO_SRC))
PROTO_GEN_HDR := $(patsubst $(PROTO_DIR)/%.proto,$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h,$(PROTO_SRC))
PROTO_OBJ     := $(patsubst %.cc,%.o,$(PROTO_GEN_CC))

# Stamp per proto
PROTO_STAMP := $(patsubst $(PROTO_DIR)/%.proto,$(OBJ_DIR)/$(PROTO_DIR)/%.stamp,$(PROTO_SRC))

# Only protos that declare a service produce gRPC stubs. Computed from the
# .proto sources themselves (not a wildcard over generated output) so the
# list is known before codegen runs and the objects actually get linked.
ifneq ($(strip $(GRPC_CPP_PLUGIN)),)
PROTO_SERVICE_SRC  := $(shell grep -l '^service ' $(PROTO_DIR)/*.proto 2>/dev/null)
PROTO_GEN_GRPC_CC  := $(patsubst $(PROTO_DIR)/%.proto,$(OBJ_DIR)/$(PROTO_DIR)/%.grpc.pb.cc,$(PROTO_SERVICE_SRC))
else
PROTO_GEN_GRPC_CC  :=
endif
PROTO_GEN_GRPC_HDR := $(patsubst %.cc,%.h,$(PROTO_GEN_GRPC_CC))
PROTO_GRPC_OBJ     := $(patsubst %.cc,%.o,$(PROTO_GEN_GRPC_CC))

# All objects (evaluated late so gRPC files are discovered)
OBJ = $(OBJ_CPP) $(PROTO_OBJ) $(PROTO_GRPC_OBJ)

# ===== Default =====
.PHONY: all
all: $(TARGET)

# ===== Link / Archive =====
ifeq ($(BUILD_SHARED),1)
$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)
else
$(TARGET): $(OBJ)
	@mkdir -p $(dir $@)
	@rm -f $@
	ar rcs $@ $(OBJ)
endif

# ===== Compile rules =====
# Normal .cpp
$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Messages
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# gRPC stubs (if exist)
$(OBJ_DIR)/$(PROTO_DIR)/%.grpc.pb.o: $(OBJ_DIR)/$(PROTO_DIR)/%.grpc.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ===== Proto codegen (messages always; gRPC optional) =====
$(OBJ_DIR)/$(PROTO_DIR)/%.stamp: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	# Always generate messages
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(OBJ_DIR)/$(PROTO_DIR)/$*.pb.h" || { echo "ERROR: missing $(OBJ_DIR)/$(PROTO_DIR)/$*.pb.h"; exit 1; }
	# Try gRPC stubs (if plugin available). If no service in proto, protoc emits nothing and still exits 0.
ifneq ($(strip $(GRPC_CPP_PLUGIN)),)
	$(PROTOC) -I . --grpc_out=$(OBJ_DIR) --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) $< || true
endif
	@touch $@

# Ensure generated files are tied to the stamp
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc:      $(OBJ_DIR)/$(PROTO_DIR)/%.stamp ;
$(OBJ_DIR)/$(PROTO_DIR)/%.grpc.pb.cc: $(OBJ_DIR)/$(PROTO_DIR)/%.stamp ;
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h:       $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;
$(OBJ_DIR)/$(PROTO_DIR)/%.grpc.pb.h:  $(OBJ_DIR)/$(PROTO_DIR)/%.grpc.pb.cc ;

# Keep generated files
GEN_FILES := $(PROTO_GEN_CC) $(PROTO_GEN_HDR) $(PROTO_GEN_GRPC_CC) $(PROTO_GEN_GRPC_HDR) $(PROTO_STAMP)
.SECONDARY: $(GEN_FILES)

# ===== Utilities =====
.PHONY: clean print docs
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

print:
	@echo "SRC_CPP           = $(SRC_CPP)"
	@echo "PROTO_SRC         = $(PROTO_SRC)"
	@echo "PROTO_GEN_CC      = $(PROTO_GEN_CC)"
	@echo "PROTO_GEN_GRPC_CC = $(PROTO_GEN_GRPC_CC)"
	@echo "OBJ               = $(OBJ)"
	@echo "TARGET            = $(TARGET)"

docs:
	doxygen Doxyfile

# ===== Header dependency tracking =====
# So that editing a .hpp invalidates every .o that (transitively) includes it,
# instead of relying only on the .cpp's own mtime.
-include $(OBJ_CPP:.o=.d)
-include $(PROTO_OBJ:.o=.d)
-include $(PROTO_GRPC_OBJ:.o=.d)
