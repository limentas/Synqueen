#include "service.grpc.pb.h"

#include "iuigateprovider.hpp"

#include "utils/corralheader.hpp"
#include <grpcpp/server.h>

namespace synqueen {

class FolderManagementGrpc final
    : public synqueen::FolderManagement::CallbackService {
public:
  FolderManagementGrpc(IUiGateProvider &provider);

  void initialize(corral::Nursery &nursery);

protected:
  virtual ::grpc::ServerUnaryReactor *
  ListFolders(::grpc::CallbackServerContext *context,
              const ::google::protobuf::Empty * /*request*/,
              ::synqueen::ListFoldersResponse *response) override;

  virtual ::grpc::ServerUnaryReactor *
  AddFolder(::grpc::CallbackServerContext * /*context*/,
            const ::synqueen::AddFolderRequest * /*request*/,
            ::synqueen::AddFolderResponse * /*response*/) override;

  virtual ::grpc::ServerUnaryReactor *
  RemoveFolder(::grpc::CallbackServerContext * /*context*/,
               const ::synqueen::RemoveFolderRequest * /*request*/,
               ::synqueen::RemoveFolderResponse * /*response*/) override;

private:
  IUiGateProvider &provider;
  corral::Nursery *nursery = nullptr;
  std::unique_ptr<grpc::Server> server;
};

} // namespace synqueen
