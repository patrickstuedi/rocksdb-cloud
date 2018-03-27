//  Copyright (c) 2016-present, Rockset, Inc.  All rights reserved.

#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "cloud/cloud_manifest.h"
#include "rocksdb/cloud/cloud_env_options.h"
#include "rocksdb/env.h"
#include "rocksdb/status.h"

namespace rocksdb {

//
// The Cloud environment
//
class CloudEnvImpl : public CloudEnv {
  friend class CloudEnv;

 public:
  // Constructor
  CloudEnvImpl(CloudType cloud_type, LogType log_type, Env* base_env);

  virtual ~CloudEnvImpl();

  const CloudType& GetCloudType() const { return cloud_type_; }
  const LogType& GetLogType() const { return log_type_; }

  // Returns the underlying env
  Env* GetBaseEnv() { return base_env_; }

  // The separator used to separate dbids while creating the dbid of a clone
  static constexpr const char* DBID_SEPARATOR = "rockset";

  // A map from a dbid to the list of all its parent dbids.
  typedef std::map<std::string, std::vector<std::string>> DbidParents;

  Status FindObsoleteFiles(const std::string& bucket_name_prefix,
                           std::vector<std::string>* pathnames);
  Status FindObsoleteDbid(const std::string& bucket_name_prefix,
                          std::vector<std::string>* dbids);
  Status extractParents(const std::string& bucket_name_prefix,
                        const DbidList& dbid_list, DbidParents* parents);

  Status LoadLocalCloudManifest(const std::string& dbname);
  // Transfers the filename from RocksDB's domain to the physical domain, based
  // on information stored in CLOUDMANIFEST.
  // For example, it will map 00010.sst to 00010.sst-[epoch] where [epoch] is
  // an epoch during which that file was created.
  // Files both in S3 and in the local directory have this [epoch] suffix.
  std::string RemapFilename(const std::string& logical_path) const;

  // This will delete all files in dest bucket and locally whose epochs are
  // invalid. For example, if we find 00010.sst-[epochX], but the real mapping
  // for 00010.sst is [epochY], in this function we will delete
  // 00010.sst-[epochX]. Note that local files are deleted immediately, while
  // cloud files are deleted with a delay of one hour (just to prevent issues
  // from two RocksDB databases running on the same bucket for a short time).
  Status DeleteInvisibleFiles(const std::string& dbname);

  CloudManifest* GetCloudManifest() { return cloud_manifest_.get(); }
  void TEST_InitEmptyCloudManifest();
  void TEST_DisableCloudManifest() { test_disable_cloud_manifest_ = true; }

 protected:
  // The type of cloud service e.g. AWS, Azure, Google,  etc.
  const CloudType cloud_type_;

  // The type of cloud service used for WAL (Kinesis, etc.)
  const LogType log_type_;

  // The dbid of the source database that is cloned
  std::string src_dbid_;

  // The pathname of the source database that is cloned
  std::string src_dbdir_;

  // The underlying env
  Env* base_env_;

  std::shared_ptr<Logger> info_log_;  // informational messages

  // Protects purger_cv_
  std::mutex purger_lock_;
  std::condition_variable purger_cv_;
  // The purger keep on running till this is set to false. (and is notified on
  // purger_cv_);
  bool purger_is_running_;
  std::thread purge_thread_;

  // A background thread that deletes orphaned objects in cloud storage
  void Purger();
  void StopPurger();

 private:
  unique_ptr<CloudManifest> cloud_manifest_;
  // This runs only in tests when we want to disable cloud manifest
  // functionality
  bool test_disable_cloud_manifest_{false};

  // scratch space in local dir
  static constexpr const char* SCRATCH_LOCAL_DIR = "/tmp";
};

}  // namespace rocksdb
