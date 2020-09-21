#include "hci_packet.h"

namespace test_vendor_lib {

// Iterator Functions
Iterator::Iterator(std::shared_ptr<HciPacket> packet, size_t i) {
  hci_packet_ = packet;
  index_ = i;
}

Iterator::Iterator(const Iterator& itr) { *this = itr; }

Iterator::operator bool() const { return hci_packet_ != nullptr; }

Iterator Iterator::operator+(size_t offset) {
  auto itr(*this);

  return itr += offset;
}

Iterator& Iterator::operator+=(size_t offset) {
  size_t new_offset = index_ + offset;
  index_ = new_offset >= hci_packet_->get_length() ? hci_packet_->get_length()
                                                   : new_offset;
  return *this;
}

Iterator Iterator::operator++(int) {
  auto itr(*this);
  index_++;

  if (index_ >= hci_packet_->get_length()) index_ = hci_packet_->get_length();

  return itr;
}

Iterator& Iterator::operator++() {
  index_++;

  if (index_ >= hci_packet_->get_length()) index_ = hci_packet_->get_length();

  return *this;
}

Iterator Iterator::operator-(size_t offset) {
  auto itr(*this);

  return itr -= offset;
}

int Iterator::operator-(Iterator& itr) { return index_ - itr.index_; }

Iterator& Iterator::operator-=(size_t offset) {
  index_ = offset > index_ ? 0 : index_ - offset;

  return *this;
}

Iterator Iterator::operator--(int) {
  auto itr(*this);
  if (index_ != 0) index_--;

  return itr;
}

Iterator& Iterator::operator--() {
  if (index_ != 0) index_--;

  return *this;
}

Iterator& Iterator::operator=(const Iterator& itr) {
  hci_packet_ = itr.hci_packet_;
  index_ = itr.index_;

  return *this;
}

bool Iterator::operator==(Iterator& itr) {
  return ((hci_packet_ == itr.hci_packet_) && (index_ == itr.index_));
}

bool Iterator::operator!=(Iterator& itr) { return !(*this == itr); }

bool Iterator::operator<(Iterator& itr) {
  return ((hci_packet_ == itr.hci_packet_) && (index_ < itr.index_));
}

bool Iterator::operator>(Iterator& itr) {
  return ((hci_packet_ == itr.hci_packet_) && (index_ > itr.index_));
}

bool Iterator::operator<=(Iterator& itr) {
  return ((hci_packet_ == itr.hci_packet_) && (index_ <= itr.index_));
}

bool Iterator::operator>=(Iterator& itr) {
  return ((hci_packet_ == itr.hci_packet_) && (index_ >= itr.index_));
}

uint8_t& Iterator::operator*() const {
  CHECK(index_ != hci_packet_->get_length());

  return hci_packet_->get_at_index(index_);
}

uint8_t* Iterator::operator->() const {
  return &hci_packet_->get_at_index(index_);
}

// BtPacket Functions
Iterator HciPacket::get_begin() {
  Iterator itr(shared_from_this(), 0);

  return itr;
}

Iterator HciPacket::get_end() {
  Iterator itr(shared_from_this(), get_length());

  return itr;
}

uint8_t HciPacket::operator[](size_t i) { return get_at_index(i); }
};  // namespace test_vendor_lib
