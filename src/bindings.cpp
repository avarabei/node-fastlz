#ifdef _WIN32
#define NOMINMAX
#endif

#include <nan.h> // pulls in Nan and v8 namespaces
#include <cmath> // std::ceil
#include <algorithm> // std::max
#include <cstdlib> // std::malloc std::realloc
#include "fastlz.h" // fastlz_*


NAN_METHOD(CompressSync) {
  if (info.Length() == 1 && node::Buffer::HasInstance(info[0])) {
    v8::Local<v8::Object> sourceObject = info[0]->ToObject();
    void* source = static_cast<void*>(node::Buffer::Data(sourceObject));

    size_t dataLength = node::Buffer::Length(info[0]);

    // Ensure the new buffer is at least big enough to hold the compressed data
    // per the requirements by fastlz
    size_t destinationSize = std::max(66, static_cast<int>(std::ceil(dataLength * 1.05)));
    void* destination = std::malloc(destinationSize);

    int compressedSize = fastlz_compress(source, dataLength, destination);

    // Reallocate the buffer so it is strictly big enough.
    char* compressedData = static_cast<char*>(std::realloc(destination, compressedSize));

    // Turn it into a node::Buffer which takes ownership of the memory and will
    // call free() on the internal buffer we allocated when it is garbage
    // collected.
    v8::Local<v8::Object> compressedBuffer = Nan::NewBuffer(compressedData, compressedSize).ToLocalChecked();
    info.GetReturnValue().Set(compressedBuffer);
  } else {
    Nan::ThrowTypeError("usage: source buffer");
  }
  return;
}

NAN_METHOD(DecompressSync) {
    if (info.Length() == 1 && node::Buffer::HasInstance(info[0])) {
        v8::Local<v8::Object> sourceObject = info[0]->ToObject();
        void* source = static_cast<void*>(node::Buffer::Data(sourceObject));

        uint32_t dataLength = node::Buffer::Length(info[0]);

        // Start with 4 x dataLength, double each time if it is not sufficient
        size_t destinationSize = std::max(66, static_cast<int>(std::ceil(dataLength * 2)));
        void* destination = nullptr;
        int compressedSize = 0;

        do {
            destinationSize <<= 1;
            destination = static_cast<char*>(std::realloc(destination, destinationSize));
            if (destination == nullptr) {
                // Allocation failed
                Nan::ThrowError("unable to allocate memory");
                return;
            }
            compressedSize = fastlz_decompress(source, dataLength, destination, destinationSize);
        } while (compressedSize == 0);


        // Reallocate the buffer so it is strictly big enough.
        char* compressedData = static_cast<char*>(std::realloc(destination, compressedSize));

        // Turn it into a node::Buffer which takes ownership of the memory and will
        // call free() on the internal buffer we allocated when it is garbage
        // collected.
        v8::Local<v8::Object> compressedBuffer = Nan::NewBuffer(compressedData, compressedSize).ToLocalChecked();
        info.GetReturnValue().Set(compressedBuffer);
    }
    else {
        Nan::ThrowTypeError("usage: source buffer");
    }
    return;
}

NAN_MODULE_INIT(InitAll) {
  Nan::Set(target, Nan::New("compressSync").ToLocalChecked(),
           Nan::GetFunction(Nan::New<v8::FunctionTemplate>(CompressSync)).ToLocalChecked());
  Nan::Set(target, Nan::New("DecompressSync").ToLocalChecked(),
      Nan::GetFunction(Nan::New<v8::FunctionTemplate>(DecompressSync)).ToLocalChecked());
}

NODE_MODULE(FastLZ, InitAll)
