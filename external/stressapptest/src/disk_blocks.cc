// Copyright 2008 Google Inc. All Rights Reserved.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//      http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Thread-safe container of disk blocks

// This file must work with autoconf on its public version,
// so these includes are correct.
#include "disk_blocks.h"

#include <utility>

// BlockData
BlockData::BlockData() : address_(0), size_(0),
                         references_(0), initialized_(false),
                         pattern_(NULL) {
  pthread_mutex_init(&data_mutex_, NULL);
}

BlockData::~BlockData() {
  pthread_mutex_destroy(&data_mutex_);
}

void BlockData::set_initialized() {
  pthread_mutex_lock(&data_mutex_);
  initialized_ = true;
  pthread_mutex_unlock(&data_mutex_);
}

bool BlockData::initialized() const {
  pthread_mutex_lock(&data_mutex_);
  bool initialized = initialized_;
  pthread_mutex_unlock(&data_mutex_);
  return initialized;
}

// DiskBlockTable
DiskBlockTable::DiskBlockTable() : sector_size_(0), write_block_size_(0),
                                   device_name_(""), device_sectors_(0),
                                   segment_size_(0), size_(0) {
  pthread_mutex_init(&data_mutex_, NULL);
  pthread_mutex_init(&parameter_mutex_, NULL);
  pthread_cond_init(&data_condition_, NULL);
}

DiskBlockTable::~DiskBlockTable() {
  pthread_mutex_destroy(&data_mutex_);
  pthread_mutex_destroy(&parameter_mutex_);
  pthread_cond_destroy(&data_condition_);
}

// 64-bit non-negative random number generator.  Stolen from
// depot/google3/base/tracecontext_unittest.cc.
int64 DiskBlockTable::Random64() {
  int64 x = random();
  x = (x << 30) ^ random();
  x = (x << 30) ^ random();
  if (x >= 0)
    return x;
  else
    return -x;
}

uint64 DiskBlockTable::Size() {
  pthread_mutex_lock(&data_mutex_);
  uint64 size = size_;
  pthread_mutex_unlock(&data_mutex_);
  return size;
}

void DiskBlockTable::InsertOnStructure(BlockData *block) {
  int64 address = block->address();
  StorageData *sd = new StorageData();
  sd->block = block;
  sd->pos = size_;
  // Creating new block ...
  pthread_mutex_lock(&data_mutex_);
  if (pos_to_addr_.size() <= size_) {
    pos_to_addr_.insert(pos_to_addr_.end(), address);
  } else {
    pos_to_addr_[size_] = address;
  }
  addr_to_block_[address] = sd;
  size_++;
  pthread_cond_broadcast(&data_condition_);
  pthread_mutex_unlock(&data_mutex_);
}

int DiskBlockTable::RemoveBlock(BlockData *block) {
  // For write threads, check the reference counter and remove
  // it from the structure.
  int64 address = block->address();
  AddrToBlockMap::iterator it = addr_to_block_.find(address);
  int ret = 1;
  if (it != addr_to_block_.end()) {
    int curr_pos = it->second->pos;
    int last_pos = size_ - 1;
    AddrToBlockMap::iterator last_it = addr_to_block_.find(
        pos_to_addr_[last_pos]);
    sat_assert(size_ > 0);
    sat_assert(last_it != addr_to_block_.end());
    // Everything is fine, removing block from table.
    pthread_mutex_lock(&data_mutex_);
    pos_to_addr_[curr_pos] = pos_to_addr_[last_pos];
    last_it->second->pos = curr_pos;
    delete it->second;
    addr_to_block_.erase(it);
    size_--;
    block->DecreaseReferenceCounter();
    if (block->GetReferenceCounter() == 0)
      delete block;
    else if (block->GetReferenceCounter() < 0)
      ret = 0;
    pthread_cond_broadcast(&data_condition_);
    pthread_mutex_unlock(&data_mutex_);
  } else {
    ret = 0;
  }
  return ret;
}

int DiskBlockTable::ReleaseBlock(BlockData *block) {
  // If caller is a random thread, just check the reference counter.
  int ret = 1;
  pthread_mutex_lock(&data_mutex_);
  int references = block->GetReferenceCounter();
  if (references == 1)
    delete block;
  else if (references > 0)
    block->DecreaseReferenceCounter();
  else
    ret = 0;
  pthread_mutex_unlock(&data_mutex_);
  return ret;
}

BlockData *DiskBlockTable::GetRandomBlock() {
  struct timespec ts;
  struct timeval tp;
  gettimeofday(&tp, NULL);
  ts.tv_sec  = tp.tv_sec;
  ts.tv_nsec = tp.tv_usec * 1000;
  ts.tv_sec += 2;  // Wait for 2 seconds.
  int result = 0;
  pthread_mutex_lock(&data_mutex_);
  while (!size_ && result != ETIMEDOUT) {
    result = pthread_cond_timedwait(&data_condition_, &data_mutex_, &ts);
  }
  if (result == ETIMEDOUT) {
    pthread_mutex_unlock(&data_mutex_);
    return NULL;
  } else {
    int64 random_number = Random64();
    int64 random_pos = random_number % size_;
    int64 address = pos_to_addr_[random_pos];
    AddrToBlockMap::const_iterator it = addr_to_block_.find(address);
    sat_assert(it != addr_to_block_.end());
    BlockData *b = it->second->block;
    // A block is returned only if its content is written on disk.
    if (b->initialized()) {
      b->IncreaseReferenceCounter();
    } else {
      b = NULL;
    }
    pthread_mutex_unlock(&data_mutex_);
    return b;
  }
}

void DiskBlockTable::SetParameters(int sector_size,
                                   int write_block_size,
                                   int64 device_sectors,
                                   int64 segment_size,
                                   const string& device_name) {
  sat_assert(size_ == 0);
  pthread_mutex_lock(&parameter_mutex_);
  sector_size_ = sector_size;
  write_block_size_ = write_block_size;
  device_sectors_ = device_sectors;
  segment_size_ = segment_size;
  device_name_ = device_name;
  pthread_mutex_unlock(&parameter_mutex_);
}

BlockData *DiskBlockTable::GetUnusedBlock(int64 segment) {
  int64 sector = 0;
  BlockData *block = new BlockData();
  bool good_sequence = false;
  if (block == NULL) {
    logprintf(0, "Process Error: Unable to allocate memory "
              "for sector data for disk %s.\n", device_name_.c_str());
    return NULL;
  }
  pthread_mutex_lock(&parameter_mutex_);
  sat_assert(device_sectors_ != 0);
  // Align the first sector with the beginning of a write block
  int num_sectors = write_block_size_ / sector_size_;
  for (int i = 0; i < kBlockRetry && !good_sequence; i++) {
    good_sequence = true;
    // Use the entire disk or a small segment of the disk to allocate the first
    // sector in the block from.
    if (segment_size_ == -1) {
      sector = (Random64() & 0x7FFFFFFFFFFFFFFFLL) % (
          device_sectors_ / num_sectors);
      sector *= num_sectors;
    } else {
      sector = (Random64() & 0x7FFFFFFFFFFFFFFFLL) % (
          segment_size_ / num_sectors);
      sector *= num_sectors;
      sector += segment * segment_size_;
      // Make sure the block is within the segment.
      if (sector + num_sectors > (segment + 1) * segment_size_) {
        good_sequence = false;
        continue;
      }
    }
    // Make sure the entire block is in range.
    if (sector + num_sectors > device_sectors_) {
      good_sequence = false;
      continue;
    }
    // Check to see if the block is free. Since the blocks are
    // now aligned to the write_block_size, it is not necessary
    // to check each sector, just the first block (a sector
    // overlap will never occur).
    pthread_mutex_lock(&data_mutex_);
    if (addr_to_block_.find(sector) != addr_to_block_.end()) {
      good_sequence = false;
    }
    pthread_mutex_unlock(&data_mutex_);
  }

  if (good_sequence) {
    block->set_address(sector);
    block->set_size(write_block_size_);
    block->IncreaseReferenceCounter();
    InsertOnStructure(block);
  } else {
    // No contiguous sequence of num_sectors sectors was found within
    // kBlockRetry iterations so return an error value.
    delete block;
    block = NULL;
  }
  pthread_mutex_unlock(&parameter_mutex_);
  return block;
}
