#ifndef GAME_SCRIPT_NETWORKEXTENSIONS_H
#define GAME_SCRIPT_NETWORKEXTENSIONS_H

namespace Compiler
{
    class Extensions;
}

namespace Interpreter
{
    class Interpreter;
}

namespace MWScript
{
    namespace Network
    {
        void installOpcodes(Interpreter::Interpreter &interpreter);
    }
}

#endif