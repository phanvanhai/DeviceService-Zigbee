SOURCE="$1"
TARGET="$2"
export CSDK_DIR=../device-sdk-c/build/release/_CPack_Packages/Linux/TGZ/csdk-1.0.0
export LD_LIBRARY_PATH=$CSDK_DIR/lib
# gcc -I$CSDK_DIR/include -L$CSDK_DIR/lib -o ${TARGET} ${SOURCE} -lcsdk
# gcc -I$CSDK_DIR/include -L$CSDK_DIR/lib -o device-random device-random.c -lcsdk user.c parson.c
gcc -I$CSDK_DIR/include -L$CSDK_DIR/lib -o device-random device-random.c -lcsdk parson.c user.c user_queue.c



# sudo fuser -k -n tcp 27017
