// Copyright (c) 2023, Jack Deeth <github@jackdeeth.org.uk>, Julia DeMille <me@jdemille.com>
// All rights reserved
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The views and conclusions contained in the software and documentation are
// those of the authors and should not be interpreted as representing official
// policies, either expressed or implied, of the FreeBSD Project.

#include "command.h"
#include "XPLMDataAccess.h"
#include "XPLMUtilities.h"
#include "log.h"
#include <string>

using namespace std::string_literals;

namespace PPL {

Command::Command(XPLMDataRef cmd_ref)
    : cmd_ref(cmd_ref)
{
}

std::optional<Command> Command::find(std::string& name)
{
    XPLMCommandRef cmd_ref = XPLMFindCommand(name.c_str());
    if (cmd_ref == nullptr) {
        return std::nullopt;
    } else {
        return Command(cmd_ref);
    }
}

Command Command::create(std::string& name, std::string& description)
{
    if (Command::find(name)) {
        throw CommandAlreadyExists(name);
    }
    XPLMCommandRef cmd_ref = XPLMCreateCommand(name.c_str(), description.c_str());
    return Command(cmd_ref);
}

CommandAlreadyExists::CommandAlreadyExists(const std::string& command)
    : std::runtime_error("The command `"s + command + "` already exists. It cannot be created."s)
{
}

template <typename T>
    requires impl_CommandHandler<T>
RegisteredCommandHandler<T> Command::handle(bool before_xp, T handler)
{
    RegisteredCommandHandler<T>(cmd_ref, before_xp, handler);
}

void Command::trigger_once()
{
    XPLMCommandOnce(cmd_ref);
}

CommandHold Command::hold_down()
{
    return CommandHold(*this);
}

CommandHold::CommandHold(Command& cmd)
    : cmd(cmd)
{
    XPLMCommandBegin(cmd.cmd_ref);
}

CommandHold::~CommandHold()
{
    XPLMCommandEnd(cmd.cmd_ref);
}

template <typename T>
    requires impl_CommandHandler<T>
RegisteredCommandHandler<T>::RegisteredCommandHandler(XPLMCommandRef cmd_ref, bool before_xp, T handler_)
    : cmd_ref(cmd_ref)
    , before_xp(before_xp)
    , handler(handler_)
{
    XPLMRegisterCommandHandler(
        cmd_ref,
        RegisteredCommandHandler<T>::handle,
        before_xp,
        &handler);
}

template <typename T>
    requires impl_CommandHandler<T>
int RegisteredCommandHandler<T>::handle(XPLMCommandRef _ref, XPLMCommandPhase phase, void* refcon)
{
    T* handler = static_cast<T*>(refcon);
    CommandOutcome outcome;
    switch (phase) {
    case xplm_CommandBegin:
        outcome = handler->command_begin();
        break;
    case xplm_CommandContinue:
        outcome = handler->command_continue();
        break;
    case xplm_CommandEnd:
        outcome = handler->command_end();
        break;
    default:
        outcome = CommandOutcome::Continue;
        Log() << Log::Error << "XPLM has called a command handler with an invalid phase!" << Log::endl;
        break;
    }
    switch (outcome) {
    case CommandOutcome::Irrelevant:
    case CommandOutcome::Continue:
        return 1;
        break;
    case CommandOutcome::Halt:
        return 0;
        break;
    }
}

template <typename T>
    requires impl_CommandHandler<T>
RegisteredCommandHandler<T>::~RegisteredCommandHandler<T>()
{
    XPLMUnregisterCommandHandler(
        cmd_ref,
        RegisteredCommandHandler<T>::handle,
        before_xp,
        &handler);
}
}
