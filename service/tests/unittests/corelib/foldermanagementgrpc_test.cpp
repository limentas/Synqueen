#include "corelib/src/uigate/foldermanagementgrpc.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace std;
using namespace std::string_literals;
using namespace synqueen;
using namespace grpc;

class MockUiGateProvider : public IUiGateProvider {
public:
  MOCK_METHOD(ListFolders, listFolders, (), (const, override));
  MOCK_METHOD(corral::Task<void>, addFolder, (const Folder &folder),
              (override));
  MOCK_METHOD(corral::Task<void>, removeFolder,
              (const std::filesystem::path &path), (override));
};

TEST(FolderManagementGrpcTest, CtorDtor) {
  MockUiGateProvider mockProvider;
  FolderManagementGrpc grpc(mockProvider);
}

class FolderManagementGrpcProxy : public FolderManagementGrpc {
public:
  FolderManagementGrpcProxy(IUiGateProvider &provider)
      : FolderManagementGrpc(provider) {}

  using FolderManagementGrpc::AddFolder;
  using FolderManagementGrpc::ListFolders;
  using FolderManagementGrpc::RemoveFolder;
};
