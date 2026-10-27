SHELL = cmd.exe
CXX ?= g++
CXXFLAGS = -Wall -Wextra -std=c++17 -pthread -Iinclude

TARGET = scheduler

SRCDIR = src
OBJDIR = obj
INCDIR = include

SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	-@if not exist $(OBJDIR) mkdir $(OBJDIR)

clean:
	-@if exist $(OBJDIR) rmdir /s /q $(OBJDIR)
	-@if exist $(TARGET).exe del /f /q $(TARGET).exe
	-@if exist $(TARGET) del /f /q $(TARGET)

.PHONY: all clean