#ifndef MAIN_HPP
#define MAIN_HPP

// transport-layer callbacks for mudmux
extern "C" int on_connect (void* ctx, int slot, void* data, size_t data_len);
extern "C" int on_transport_ready (void* ctx, int slot, void* data, size_t data_len);
extern "C" int on_disconnect (void* ctx, int slot, void* data, size_t data_len);

#endif
