LIBS=-F/System/Library/PrivateFrameworks -framework MultitouchSupport \
	 -framework CoreFoundation \
	 -framework Carbon

BUILD_DIR=build
UTIL=trackpad_mapper_util
TARGET=trackpad_mapper
APP="Trackpad Mapper.app"

all:
	if [[ ! -e ${BUILD_DIR} ]]; then mkdir -p ${BUILD_DIR}; fi
	gcc ${LIBS} ${UTIL}.c -o ${BUILD_DIR}/${UTIL} -g
	swiftc ${TARGET}.swift -o ${BUILD_DIR}/${TARGET} -g
	make app

release:
	if [[ ! -e ${BUILD_DIR} ]]; then mkdir -p ${BUILD_DIR}; fi
	gcc ${LIBS} ${UTIL}.c -o ${BUILD_DIR}/${UTIL} -O3
	swiftc ${TARGET}.swift -o ${BUILD_DIR}/${TARGET} -O3
	make app

app:
	@if [[ ! -e ${BUILD_DIR}/${APP} ]]; then \
		echo Creating app bundle; \
		mkdir -p ${BUILD_DIR}/${APP}/Contents/MacOS; \
		echo done; \
	fi
	@cp -R Resources ${BUILD_DIR}/${APP}/Contents/Resources
	@cp Info.plist ${BUILD_DIR}/${APP}/Contents
	@cp ${BUILD_DIR}/${UTIL} ${BUILD_DIR}/${APP}/Contents/MacOS
	@cp ${BUILD_DIR}/${TARGET} ${BUILD_DIR}/${APP}/Contents/MacOS

install:
	cp -R ${BUILD_DIR}/${APP} /Applications
