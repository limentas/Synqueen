#include "foldermanagementgrpc.hpp"

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <spdlog/spdlog.h>
using namespace grpc;

namespace synqueen {

FolderManagementGrpc::FolderManagementGrpc(IUiGateProvider &provider)
    : provider(provider) {}

void FolderManagementGrpc::initialize(corral::Nursery &nursery) {
  this->nursery = &nursery;

  // TODO: make this configurable
  std::string server_address("0.0.0.0:50051");
  ServerBuilder builder;
  builder.AddListeningPort(server_address, InsecureServerCredentials());
  builder.RegisterService(this);
  server = builder.BuildAndStart();
  if (server == nullptr) {
    throw std::runtime_error("Failed to start gRPC server");
  }
  SPDLOG_INFO("Server listening on {}", server_address);
}

ServerUnaryReactor *
FolderManagementGrpc::ListFolders(CallbackServerContext *context,
                                  const ::google::protobuf::Empty * /*request*/,
                                  ::synqueen::ListFoldersResponse *response) {
  auto *reactor = context->DefaultReactor();
  auto folders = provider.listFolders();
  for (const auto &folder : folders) {
    auto *f = response->mutable_folders()->Add();
    f->set_path(folder.path.string());
  }
  reactor->Finish(::grpc::Status::OK);
  return reactor;
}

ServerUnaryReactor *
FolderManagementGrpc::AddFolder(CallbackServerContext *context,
                                const ::synqueen::AddFolderRequest *request,
                                ::synqueen::AddFolderResponse *response) {
  auto *reactor = context->DefaultReactor();
  auto path = request->path();
  nursery->start([this, reactor, response, path]() -> corral::Task<void> {
    try {
      co_await provider.addFolder(
          IUiGateProvider::Folder{std::filesystem::path(path)});
      response->set_result(AR_SUCCESS);
      reactor->Finish(::grpc::Status::OK);
    } catch (const std::exception &e) {
      SPDLOG_ERROR("Failed to add folder {}: {}", path, e.what());
      response->set_result(AR_UNKNOWN_ERROR);
      reactor->Finish(::grpc::Status(::grpc::StatusCode::INTERNAL, e.what()));
      co_return;
    }
  });
  return reactor;
}

ServerUnaryReactor *FolderManagementGrpc::RemoveFolder(
    CallbackServerContext * /*context*/,
    const ::synqueen::RemoveFolderRequest * /*request*/,
    ::synqueen::RemoveFolderResponse * /*response*/) {
  return nullptr;
}

} // namespace synqueen
