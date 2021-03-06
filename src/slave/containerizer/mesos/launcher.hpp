// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __LAUNCHER_HPP__
#define __LAUNCHER_HPP__

#include <sys/types.h>

#include <list>
#include <map>
#include <string>

#include <mesos/mesos.hpp>

#include <mesos/slave/isolator.hpp>

#include <process/future.hpp>
#include <process/subprocess.hpp>

#include <stout/flags.hpp>
#include <stout/hashmap.hpp>
#include <stout/hashset.hpp>
#include <stout/lambda.hpp>
#include <stout/option.hpp>
#include <stout/try.hpp>

#include "slave/flags.hpp"

namespace mesos {
namespace internal {
namespace slave {

class Launcher
{
public:
  virtual ~Launcher() {}

  // Recover the necessary state for each container listed in state.
  // Return the set of containers that are known to the launcher but
  // not known to the slave (a.k.a. orphans).
  virtual process::Future<hashset<ContainerID>> recover(
      const std::list<mesos::slave::ContainerState>& states) = 0;

  // Fork a new process in the containerized context. The child will
  // exec the binary at the given path with the given argv, flags and
  // environment. The I/O of the child will be redirected according to
  // the specified I/O descriptors. The parentHooks will be executed
  // in the parent process before the child execs. The parent will return
  // the child's pid if the fork is successful.
  virtual Try<pid_t> fork(
      const ContainerID& containerId,
      const std::string& path,
      const std::vector<std::string>& argv,
      const process::Subprocess::IO& in,
      const process::Subprocess::IO& out,
      const process::Subprocess::IO& err,
      const flags::FlagsBase* flags,
      const Option<std::map<std::string, std::string>>& environment,
      const Option<int>& namespaces,
      std::vector<process::Subprocess::ParentHook> parentHooks = {}) = 0;

  // Kill all processes in the containerized context.
  virtual process::Future<Nothing> destroy(const ContainerID& containerId) = 0;

  // Return ContainerStatus information about container.
  // Currently only returns Executor PID info.
  virtual process::Future<ContainerStatus> status(
      const ContainerID& containerId) = 0;
};


// Launcher suitable for any POSIX compliant system. Uses process
// groups and sessions to track processes in a container. POSIX states
// that process groups cannot migrate between sessions so all
// processes for a container will be contained in a session.
class PosixLauncher : public Launcher
{
public:
  static Try<Launcher*> create(const Flags& flags);

  virtual ~PosixLauncher() {}

  virtual process::Future<hashset<ContainerID>> recover(
      const std::list<mesos::slave::ContainerState>& states);

  virtual Try<pid_t> fork(
      const ContainerID& containerId,
      const std::string& path,
      const std::vector<std::string>& argv,
      const process::Subprocess::IO& in,
      const process::Subprocess::IO& out,
      const process::Subprocess::IO& err,
      const flags::FlagsBase* flags,
      const Option<std::map<std::string, std::string>>& environment,
      const Option<int>& namespaces,
      std::vector<process::Subprocess::ParentHook> parentHooks = {});

  virtual process::Future<Nothing> destroy(const ContainerID& containerId);

  virtual process::Future<ContainerStatus> status(
      const ContainerID& containerId);

protected:
  PosixLauncher() {}

  // The 'pid' is the process id of the first process and also the
  // process group id and session id.
  hashmap<ContainerID, pid_t> pids;
};


// Minimal implementation of a `Launcher` for the Windows platform. Does not
// take into account process groups (jobs) or sessions.
class WindowsLauncher : public PosixLauncher
{
public:
  static Try<Launcher*> create(const Flags& flags);

  virtual ~WindowsLauncher() {}

private:
  WindowsLauncher() {}
};

} // namespace slave {
} // namespace internal {
} // namespace mesos {

#endif // __LAUNCHER_HPP__
