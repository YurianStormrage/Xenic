#pragma once

#include <cstdint>

void dma_read(uint64_t addr, uint64_t size, void *buffer);
void dma_write(uint64_t addr, uint64_t size, void *buffer);

auto log_write();
