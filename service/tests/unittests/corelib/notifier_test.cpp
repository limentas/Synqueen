#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "utils/notifier.hpp"

using namespace testing;

TEST(NotifierTest, BasicNotification) {
  synqueen::utils::Notifier notifier;
  bool notified = false;
  notifier.addSyncHandler([&]() { notified = true; });
  notifier.notify();
  EXPECT_TRUE(notified);
}
