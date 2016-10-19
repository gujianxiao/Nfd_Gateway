/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_DAEMON_MGMT_MANAGER_BASE_HPP
#define NFD_DAEMON_MGMT_MANAGER_BASE_HPP

#include "common.hpp"
#include "mgmt/command-validator.hpp"

#include <ndn-cxx/mgmt/dispatcher.hpp>
#include <ndn-cxx/management/nfd-control-command.hpp>
#include <ndn-cxx/management/nfd-control-response.hpp>
#include <ndn-cxx/management/nfd-control-parameters.hpp>

namespace nfd {

using ndn::mgmt::Dispatcher;

using ndn::nfd::ControlCommand;
using ndn::nfd::ControlResponse;
using ndn::nfd::ControlParameters;

/**
 * @brief a collection of common functions shared by all NFD managers,
 *        such as communicating with the dispatcher and command validator.
 */
class ManagerBase : public noncopyable
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

public:
  ManagerBase(Dispatcher& dispatcher,
              CommandValidator& validator,
              const std::string& module);

PUBLIC_WITH_TESTS_ELSE_PROTECTED: // registrations to the dispatcher

  // difference from mgmt::ControlCommand: accepts nfd::ControlParameters
  typedef function<void(const ControlCommand& command,
                        const Name& prefix, const Interest& interest,
                        const ControlParameters& parameters,
                        const ndn::mgmt::CommandContinuation done)> ControlCommandHandler;

  template<typename Command>
  void
  registerCommandHandler(const std::string& verb,
                         const ControlCommandHandler& handler);

  void
  registerStatusDatasetHandler(const std::string& verb,
                               const ndn::mgmt::StatusDatasetHandler& handler);

  ndn::mgmt::PostNotification
  registerNotificationStream(const std::string& verb);

PUBLIC_WITH_TESTS_ELSE_PRIVATE: // command validation
  /**
   * @brief validate a request for ControlCommand.
   *
   * This is called by the dispatcher.
   *
   * @pre params != null
   * @pre typeid(*params) == typeid(ndn::nfd::ControlParameters)
   *
   * @param prefix the top prefix
   * @param interest a request for ControlCommand
   * @param params the parameters for ControlCommand
   * @param accept callback of successful validation, take the requester string as a argument
   * @param reject callback of failure in validation, take the action code as a argument
   */
  void
  authorize(const Name& prefix, const Interest& interest,
            const ndn::mgmt::ControlParameters* params,
            ndn::mgmt::AcceptContinuation accept,
            ndn::mgmt::RejectContinuation reject);

  /**
   * @brief extract a requester from a ControlCommand request
   *
   * This is called after the signature is validated.
   *
   * @param interest a request for ControlCommand
   * @param accept callback of successful validation, take the requester string as a argument
   */
  void
  extractRequester(const Interest& interest,
                   ndn::mgmt::AcceptContinuation accept);

PUBLIC_WITH_TESTS_ELSE_PRIVATE: // helpers
  /**
   * @brief validate the @p parameters for a given @p command
   *
   * @param parameters the original ControlParameters
   *
   * @return whether the original ControlParameters can be validated
   */
  static bool
  validateParameters(const nfd::ControlCommand& command,
                     const ndn::mgmt::ControlParameters& parameters);

  /** @brief Handle control command
   */
  static void
  handleCommand(shared_ptr<nfd::ControlCommand> command,
                const ControlCommandHandler& handler,
                const Name& prefix, const Interest& interest,
                const ndn::mgmt::ControlParameters& params,
                ndn::mgmt::CommandContinuation done);

  /**
   * @brief generate the relative prefix for a handler,
   *        by appending the verb name to the module name.
   *
   * @param verb the verb name
   *
   * @return the generated relative prefix
   */
  PartialName
  makeRelPrefix(const std::string& verb);

private:
  Dispatcher&       m_dispatcher;
  CommandValidator& m_validator;
  std::string       m_mgmtModuleName;
};

inline PartialName
ManagerBase::makeRelPrefix(const std::string& verb)
{
  return PartialName(m_mgmtModuleName).append(verb);
}

template<typename Command>
inline void
ManagerBase::registerCommandHandler(const std::string& verb,
                                    const ControlCommandHandler& handler)
{
  auto command = make_shared<Command>();

  m_dispatcher.addControlCommand<ControlParameters>(
    makeRelPrefix(verb),
    bind(&ManagerBase::authorize, this, _1, _2, _3, _4, _5),
    bind(&ManagerBase::validateParameters, cref(*command), _1),
    bind(&ManagerBase::handleCommand, command, handler, _1, _2, _3, _4));
}

} // namespace nfd

#endif // NFD_DAEMON_MGMT_MANAGER_BASE_HPP