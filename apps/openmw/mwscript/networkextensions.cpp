#include "networkextensions.hpp"

#include <boost/lexical_cast.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/compiler/opcodes.hpp>

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

        class OpNetworkOpen : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                using boost::lexical_cast;

                int port = runtime.getIntegerLiteral(runtime[0].mInteger);
                runtime.pop();

                std::string protocol = runtime.getStringLiteral(runtime[0].mInteger);
                runtime.pop();

                runtime.getContext().report( "Opening a server on port " + lexical_cast<std::string>(port) + "." );
                MWBase::Environment::get().getWorld()->getNetwork().openServer(port, protocol);
                runtime.getContext().report("Server open.");
            }
        };

        class OpNetworkClose : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWBase::Environment::get().getWorld()->getNetwork().closeServer();
                runtime.getContext().report("Server Closed");
            }
        };


        void installOpcodes(Interpreter::Interpreter &interpreter)
        {
            interpreter.installSegment5(Compiler::Network::opcodeNetworkConnect, new OpNetworkConnect);
            interpreter.installSegment5(Compiler::Network::opcodeNetworkOpen, new OpNetworkOpen);
            interpreter.installSegment5(Compiler::Network::opcodeNetworkClose, new OpNetworkClose);
        }
    }
}