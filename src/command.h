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

#ifndef PPL_COMMAND_H_
#define PPL_COMMAND_H_

#include <XPLMUtilities.h>

#include <concepts>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>

// RAII wrapper for X-Plane Commands API

namespace PPL {

class CommandHandler;

template <typename T>
concept impl_CommandHandler = std::derived_from<T, CommandHandler> && (!std::same_as<T, CommandHandler>);

template <typename T>
    requires impl_CommandHandler<T>
class RegisteredCommandHandler;

class CommandHold;

class Command {
    friend CommandHold;

public:
    static std::optional<Command> find(std::string& name);
    static Command create(std::string& name, std::string& description);
    template <typename T>
        requires impl_CommandHandler<T>
    RegisteredCommandHandler<T> handle(bool before_xp, T handler);
    void trigger_once();
    CommandHold hold_down();

private:
    Command(XPLMCommandRef cmd_ref);
    XPLMCommandRef cmd_ref;
};

class CommandAlreadyExists : public std::runtime_error {
    friend Command;

private:
    CommandAlreadyExists(const std::string& command);
};

class CommandHold {
    friend Command;

public:
    ~CommandHold();

private:
    CommandHold(Command& cmd);
    Command& cmd;
};

enum class CommandOutcome {
    /**
     * @brief Allow X-Plane to handle the command.
     */
    Continue,
    /**
     * @brief Prevent X-Plane from handling the command.
     */
    Halt,
    /**
     * @brief Return this if handling the command after X-Plane.
     */
    Irrelevant
};

enum class CommandPhase {
    /**
     * @brief The command has begun.
     */
    Begin,
    /**
     * @brief Periodic events with this phase are sent when the command is held.
     */
    Continue,
    /**
     * @brief The command has been released.
     */
    End
};

class CommandHandler {
public:
    CommandHandler();
    virtual ~CommandHandler() = 0;
    virtual CommandOutcome command_begin() = 0;
    virtual CommandOutcome command_continue() = 0;
    virtual CommandOutcome command_end() = 0;
};

template <typename T>
    requires impl_CommandHandler<T>
class RegisteredCommandHandler {
    friend Command;

public:
    ~RegisteredCommandHandler();

private:
    RegisteredCommandHandler(XPLMCommandRef cmd_ref, bool before_xp, T handler);
    XPLMCommandRef cmd_ref;
    bool before_xp;
    T handler;
    static int handle(XPLMCommandRef ref, XPLMCommandPhase phase, void* refcon);
};

} // namespace

#endif // PPL_COMMAND_H_
