BUILD_DIR = out
BIN = ${BUILD_DIR}/formatexe

export VCPKG_ROOT

DEBUG_FLAGS = -DCMAKE_BUILD_TYPE=Debug
RELEASE_FLAGS = -DCMAKE_BUILD_TYPE=Release

run: ${BIN}
	./${BIN}

wrun: ${BIN}.exe
	.\${BIN}.exe

${BIN}: ${BUILD_DIR}/ debug

${BIN}.exe: ${BUILD_DIR}/ debug

debug: ${BUILD_DIR}/
	cd ${BUILD_DIR} && \
	cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" ${DEBUG_FLAGS} && \
	cmake --build .

release: ${BUILD_DIR}/
	cd ${BUILD_DIR} && \
	cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE="${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" ${RELEASE_FLAGS} && \
	cmake --build .

${BUILD_DIR}/:
	mkdir ${BUILD_DIR}
