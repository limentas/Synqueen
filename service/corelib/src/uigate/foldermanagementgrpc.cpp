#include "foldermanagementgrpc.hpp"

#include "utils/exceptions.hpp"
#include "utils/taskscheduler.hpp"

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

ServerUnaryReactor *FolderManagementGrpc::ListFolders(
    CallbackServerContext *context,
    const ::google::protobuf::Empty * /*request*/,
    ::synqueen::v1::ListFoldersResponse *response) {
  auto *reactor = context->DefaultReactor();
  TaskScheduler::runOnMainThread([this, reactor, response]() {
    try {
      auto folders = provider.listFolders();
      for (const auto &folder : folders) {
        auto *f = response->mutable_folders()->Add();
        f->set_path(folder.path.string());
      }
      reactor->Finish(::grpc::Status::OK);
    } catch (const std::exception &e) {
      SPDLOG_ERROR("Uncaught exception occurred in ListFolders task: {}",
                   e.what());
      reactor->Finish(::grpc::Status(::grpc::StatusCode::INTERNAL,
                                     "Unknown internal error"));
    }
  });
  return reactor;
}

ServerUnaryReactor *
FolderManagementGrpc::AddFolder(CallbackServerContext *context,
                                const ::synqueen::v1::AddFolderRequest *request,
                                ::google::protobuf::Empty *response) {
  assert(nursery != nullptr);
  auto *reactor = context->DefaultReactor();
  auto path = request->path();
  TaskScheduler::runOnMainThreadAsync([this, reactor, response,
                                       path]() -> corral::Task<void> {
    try {
      co_await provider.addFolder(
          IUiGateProvider::Folder{std::filesystem::path(path)});
      reactor->Finish(::grpc::Status::OK);
      co_return;
    } catch (const SqDoesNotExist &e) {
      SPDLOG_ERROR("Failed to add folder {}: {}", path, e.what());
      reactor->Finish(
          ::grpc::Status(::grpc::StatusCode::NOT_FOUND, "Folder not found"));
    } catch (const SqAlreadyUsed &e) {
      SPDLOG_ERROR("Failed to add folder {}: {}", path, e.what());
      reactor->Finish(::grpc::Status(::grpc::StatusCode::ALREADY_EXISTS,
                                     "Folder already used"));
    } catch (const SqPermissionDenied &e) {
      SPDLOG_ERROR("Failed to add folder {}: {}", path, e.what());
      reactor->Finish(::grpc::Status(::grpc::StatusCode::PERMISSION_DENIED,
                                     "Permission denied"));
    } catch (const SqNotADirectory &e) {
      SPDLOG_ERROR("Failed to add folder {}: {}", path, e.what());
      reactor->Finish(::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION,
                                     "Path is not a directory"));
    } catch (const std::exception &e) {
      SPDLOG_ERROR("Failed to add folder {}: {}", path, e.what());
      reactor->Finish(::grpc::Status(::grpc::StatusCode::INTERNAL,
                                     "Unknown internal error"));
    }
  });
  return reactor;
}

ServerUnaryReactor *FolderManagementGrpc::RemoveFolder(
    CallbackServerContext * /*context*/,
    const ::synqueen::v1::RemoveFolderRequest * /*request*/,
    ::google::protobuf::Empty * /*response*/) {
  // TODO: implement
  return nullptr;
}

} // namespace synqueen
