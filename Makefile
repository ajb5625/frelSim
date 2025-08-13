# ===== Config =====
CXX        := g++
PROTOC     := protoc

# 0 = static (.a), 1 = shared (.so)
BUILD_SHARED ?= 0
DEBUG        ?= 1

# Project layout
SRC_DIRS    := frelsim
PROTO_DIR   := frelsim/type/proto
OBJ_DIR     := build
BIN_DIR     := build
LIB_NAME    := frelsim

# Probe protobuf cflags/libs (fallbacks if pkg-config missing)
PROTOBUF_CFLAGS := $(shell pkg-config --cflags protobuf 2>/dev/null)
PROTOBUF_LIBS   := $(shell pkg-config --libs protobuf 2>/dev/null)
ifeq ($(PROTOBUF_CFLAGS),)
  PROTOBUF_CFLAGS := -I/usr/include
endif
ifeq ($(PROTOBUF_LIBS),)
  PROTOBUF_LIBS := -lprotobuf
endif

# ===== Flags =====
WARN     := -Wall -Wextra -Wconversion -Wshadow
STD      := -std=c++20
ifeq ($(DEBUG),1)
  OPT   := -O0 -g3
else
  OPT   := -O3 -g0
endif

# Include build/ so you can `#include "frelsim/type/proto/X.pb.h"`
CXXFLAGS := $(STD) $(WARN) $(OPT) -Iinclude -I$(OBJ_DIR) $(PROTOBUF_CFLAGS)
LDFLAGS  :=
LDLIBS   := $(PROTOBUF_LIBS)

ifeq ($(BUILD_SHARED),1)
  CXXFLAGS += -fPIC
  TARGET    := $(BIN_DIR)/lib$(LIB_NAME).so
  LDFLAGS   += -shared
else
  TARGET    := $(BIN_DIR)/lib$(LIB_NAME).a
endif

# ===== Sources =====
# Normal .cpp files under frelsim (ignore any generated pb.cc in source tree)
SRC_CPP  := $(shell find $(SRC_DIRS) -type f -name '*.cpp' ! -name '*.pb.cpp' ! -name '*.pb.cc' 2>/dev/null)
OBJ_CPP  := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRC_CPP))

# Protos — keep it simple
PROTO_SRC     := $(wildcard $(PROTO_DIR)/*.proto)
PROTO_GEN_CC  := $(patsubst $(PROTO_DIR)/%.proto,$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc,$(PROTO_SRC))
PROTO_GEN_HDR := $(patsubst $(PROTO_DIR)/%.proto,$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h,$(PROTO_SRC))
PROTO_OBJ     := $(patsubst %.cc,%.o,$(PROTO_GEN_CC))

OBJ           := $(OBJ_CPP) $(PROTO_OBJ)

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
# Normal .cpp -> build/…/.o
$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;$(OBJ_DIR)/%.o: %.cpp | $(PROTO_GEN_HDR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Generated protobuf .pb.cc -> .o
$(OBJ_DIR)/$(PROTO_DIR)/%.o: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .proto -> build/…/.pb.cc (protoc also emits .pb.h)
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc: $(PROTO_DIR)/%.proto
	@mkdir -p $(dir $@)
	$(PROTOC) -I . --cpp_out=$(OBJ_DIR) $<
	@test -f "$(@:.cc=.h)" || { echo "ERROR: missing header $(@:.cc=.h)"; exit 1; }

# Header is produced together with .pb.cc; no recipe
$(OBJ_DIR)/$(PROTO_DIR)/%.pb.h: $(OBJ_DIR)/$(PROTO_DIR)/%.pb.cc ;


# Keep generated protobuf files; don't auto-delete as intermediates
GEN_FILES := $(PROTO_GEN_CC) $(PROTO_GEN_HDR)

.SECONDARY: $(GEN_FILES)     # or use .PRECIOUS if you prefer
# .PRECIOUS: $(GEN_FILES)


# ===== Utilities =====
.PHONY: clean print
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

print:
	@echo "SRC_CPP     = $(SRC_CPP)"
	@echo "PROTO_SRC   = $(PROTO_SRC)"
	@echo "PROTO_GEN_CC= $(PROTO_GEN_CC)"
	@echo "OBJ         = $(OBJ)"
	@echo "TARGET      = $(TARGET)"
