# Compiler and standard flags
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 \
-I/home/shreya/libs/json-full/single_include \
-Wno-unused-parameter -Wno-unknown-pragmas \
-Wno-unused-function -Wno-sign-compare -Wno-unused-variable \
-Wno-missing-field-initializers



# Include directories
INCLUDES = \
  -I. \
  -I/usr/local/include \
  -I/usr/local/include/openfhe \
  -I/usr/local/include/openfhe/pke \
  -I/usr/local/include/openfhe/core \
  -I/usr/local/include/openfhe/binfhe \
  -I/home/shreya/libs/json-full/single_include

# Libraries - verify your install path
LIBS = \
  /usr/local/lib/libOPENFHEpke.so \
  /usr/local/lib/libOPENFHEcore.so \
  /usr/local/lib/libOPENFHEbinfhe.so \
  -lcurl -lpthread -lntl -lgmp -lm

# Common utility source files
UTIL_SRCS = base64_utils.cpp curl_utils.cpp serialization_utils.cpp rest_storage.cpp
UTIL_OBJS = $(UTIL_SRCS:.cpp=.o)

# CryptoContext source files
CC_SRCS = cc.cpp cc_registry.cpp
CC_OBJS = $(CC_SRCS:.cpp=.o)

# Mongoose source
MONGOOSE_SRCS = mongoose.c
MONGOOSE_OBJS = $(MONGOOSE_SRCS:.c=.o)

# Executables (updated to your current filenames)
TARGETS = \
  cc \
  client1_keygen client2_keygen \
  client1_rekeygen client2_rekeygen \
  client1_encrypt client2_encrypt \
  client1_decrypt client2_decrypt \
  api_server \
  operations

# Default build target
all: $(TARGETS)

# Compile utility object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile cc object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile mongoose object file
%.o: %.c
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Build targets and link application binaries

cc: $(CC_OBJS) $(UTIL_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

client1_keygen: client1_keygen.cpp cc_registry.cpp $(UTIL_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

client2_keygen: client2_keygen.cpp cc_registry.cpp $(UTIL_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

client1_rekeygen: client1_rekeygen.cpp cc_registry.cpp $(UTIL_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

client2_rekeygen: client2_rekeygen.cpp cc_registry.cpp $(UTIL_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

client1_encrypt: client1_encrypt.cpp cc_registry.cpp $(UTIL_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

client2_encrypt: client2_encrypt.cpp cc_registry.cpp $(UTIL_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

client1_decrypt: client1_decrypt.cpp cc_registry.cpp $(UTIL_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

client2_decrypt: client2_decrypt.cpp cc_registry.cpp $(UTIL_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

api_server: api_server.cpp mongoose.c $(UTIL_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) \
	-DMG_MAX_RECV_SIZE=104857600 \
	-DMG_MAX_HTTP_REQUEST_SIZE=104857600 \
	-DMG_MAX_UPLOAD_SIZE=104857600 \
	$^ -o $@ $(LIBS)

operations: operations.cpp cc_registry.cpp $(UTIL_OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ -o $@ $(LIBS)

# Clean up generated binaries and object files, logs, keys, etc.
clean:
	rm -f *.o $(TARGETS) \
	    *.key *.ct *.bin *.log *.json \
	    client1_data/*.json client1_data/*.pkl client1_data/*.key client2_data/*.json client2_data/*.pkl client2_data/*.key \
	    logs/*.txt logs/*.json
