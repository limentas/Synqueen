#pragma once

#include "iuigateprovider.hpp"
#include "service.grpc.pb.h"
#include "utils/corralheader.hpp"

#include <grpcpp/server.h>
#include <memory>

namespace synqueen {

class FolderManagementGrpc
    : public ::synqueen::v1::FolderManagement::CallbackService {
public:
  FolderManagementGrpc(IUiGateProvider &provider);

  void initialize(corral::Nursery &nursery);

protected:
  virtual ::grpc::ServerUnaryReactor *
  ListFolders(::grpc::CallbackServerContext *context,
              const ::google::protobuf::Empty * /*request*/,
              ::synqueen::v1::ListFoldersResponse *response) override;

  virtual ::grpc::ServerUnaryReactor *
  AddFolder(::grpc::CallbackServerContext * /*context*/,
            const ::synqueen::v1::AddFolderRequest * /*request*/,
            ::google::protobuf::Empty * /*response*/) override;

  virtual ::grpc::ServerUnaryReactor *
  RemoveFolder(::grpc::CallbackServerContext * /*context*/,
               const ::synqueen::v1::RemoveFolderRequest * /*request*/,
               ::google::protobuf::Empty * /*response*/) override;

private:
  IUiGateProvider &provider;
  corral::Nursery *nursery = nullptr;
  std::unique_ptr<grpc::Server> server;
};

} // namespace synqueen
