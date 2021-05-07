SRCDIR = src
OBJDIR = obj
BINDIR = bin
SRC += $(wildcard $(SRCDIR)/*.cpp)
OBJ += $(SRC:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
DEP += $(OBJ:.o=.d)

CXX = g++
CXXFLAGS = -g --std=gnu++17 -MMD -Wall -Wunused -Wpointer-arith -I./src
CXXFLAGS += -O2
LDFLAGS =
dir_guard=@mkdir -p $(@D)

#CXXFLAGS += -fsanitize=address -fsanitize=undefined
#LDFLAGS += -lasan

.PHONY: all
all: $(BINDIR)/acramalpha

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(dir_guard)
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)

$(BINDIR)/acramalpha: $(OBJ)
	$(dir_guard)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

-include $(DEP)