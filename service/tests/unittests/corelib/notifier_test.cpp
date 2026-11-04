#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "utils/notifier.hpp"

using namespace testing;

TEST(NotifierTest, NoHandlers) {
  synqueen::utils::Notifier notifier;
  // Should not throw or crash when notifying with no handlers
  EXPECT_NO_THROW(notifier.notify());
}

TEST(NotifierTest, BasicNotification) {
  synqueen::utils::Notifier notifier;
  bool notified = false;
  notifier.addSyncHandler(&notified, [&]() { notified = true; });
  notifier.notify();
  EXPECT_TRUE(notified);
}

TEST(NotifierTest, MultipleNotifications) {
  synqueen::utils::Notifier notifier;
  int notifyCount = 0;
  notifier.addSyncHandler(&notifyCount, [&]() { notifyCount++; });
  notifier.notify();
  notifier.notify();
  EXPECT_EQ(notifyCount, 2);
}

TEST(NotifierTest, MultipleHandlers) {
  synqueen::utils::Notifier notifier;
  int notifyCount1 = 0;
  int notifyCount2 = 0;
  notifier.addSyncHandler(&notifyCount1, [&]() { notifyCount1++; });
  notifier.addSyncHandler(&notifyCount2, [&]() { notifyCount2++; });
  notifier.notify();
  EXPECT_EQ(notifyCount1, 1);
  EXPECT_EQ(notifyCount2, 1);
}

TEST(NotifierTest, HandlerRemoval) {
  synqueen::utils::Notifier notifier;
  int notifyCount = 0;
  auto handler = [&]() { notifyCount++; };
  notifier.addSyncHandler(&handler, handler);
  notifier.removeSyncHandler(&handler);
  notifier.notify();
  EXPECT_EQ(notifyCount, 0);
}

TEST(NotifierTest, HandlerRemovalWithMultipleHandlers) {
  synqueen::utils::Notifier notifier;
  int notifyCount1 = 0;
  int notifyCount2 = 0;
  auto handler1 = [&]() { notifyCount1++; };
  auto handler2 = [&]() { notifyCount2++; };
  notifier.addSyncHandler(&handler1, handler1);
  notifier.addSyncHandler(&handler2, handler2);
  notifier.removeSyncHandler(&handler1);
  notifier.notify();
  EXPECT_EQ(notifyCount1, 0);
  EXPECT_EQ(notifyCount2, 1);
}
