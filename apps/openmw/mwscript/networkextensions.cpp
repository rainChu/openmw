#include "networkextensions.hpp"

#include <components/interpreter/interpreter.hpp>
#include <components/compiler/opcodes.hpp>

#include <components/interpreter/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/network.hpp"

#include "interpretercontext.hpp"

namespace MWScript
{
    namespace Network
    {
        class OpNetworkConnect : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                std::string address = runtime.getStringLiteral (runtime[0].mInteger);
                runtime.pop();

                std::string password = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();

                runtime.getContext().report("Connecting to <" + address + "> ...");
                MWBase::Environment::get().getWorld()->getNetwork().connect(address, password);
                runtime.getContext().report("...Not implemented!");
            }
        };


        void installOpcodes(Interpreter::Interpreter &interpreter)
        {
            interpreter.installSegment5(Compiler::Network::opcodeNetworkConnect, new OpNetworkConnect);
        }
    }
}