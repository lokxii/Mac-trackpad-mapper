LIBS=-F/System/Library/PrivateFrameworks -framework MultitouchSupport \
	 -framework CoreFoundation \
	 -framework Carbon

TARGET=trackpad_mapper

all:
	gcc ${LIBS} main.c -o ${TARGET} -g

release:
	gcc ${LIBS} main.c -o ${TARGET} -g -O3
