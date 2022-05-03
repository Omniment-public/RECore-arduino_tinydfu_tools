CC = clang++-14
CFLAGS    =
TARGET  = RECore_uart_writer
TARGET_CROSS  = RECore_uart_writer_aarc64
TARGET_NOAUTO  = RECore_uart_writer_noauto_aarc64
SRCS    = RECore_uart_writer_linux.cpp
SRCS_NOAUTO    = RECore_uart_writer_linux_noauto.cpp
OBJS    = $(SRCS:.cpp=.o)
OPTA	= --target=aarch64-linux-gnu

all: $(TARGET) $(TARGET_CROSS) $(TARGET_NOAUTO)

$(TARGET): $(SRCS)
	$(CC) -o $(TARGET) $(SRCS)

$(TARGET_CROSS): $(SRCS)
	$(CC) -o $(TARGET_CROSS) $(SRCS) $(OPTA)

$(TARGET_NOAUTO): $(SRCS_NOAUTO)
	$(CC) -o $(TARGET_NOAUTO) $(SRCS_NOAUTO) $(OPTA)

#RECore_uart_writer: RECore_uart_writer_linux.cpp
#	clang++-14 -o RECore_uart_writer RECore_uart_writer_linux.cpp

#RECore_uart_writer_aarc64: RECore_uart_writer_linux.cpp
#	clang++-14 -o RECore_uart_writer_aarc64 RECore_uart_writer_linux.cpp --target=aarch64-linux-gnu
