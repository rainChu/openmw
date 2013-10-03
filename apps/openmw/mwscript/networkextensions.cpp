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
        class OpNetworkJoin : public Interpreter::Opcode0
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
                runtime.getContext().report("Connected.");
            }
        };

        class OpNetworkCreate : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                using boost::lexical_cast;

                int port = runtime[0].mInteger;
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
                MWBase::Environment::get().getWorld()->getNetwork().close();
                runtime.getContext().report("Server Closed");
            }
        };


        void installOpcodes(Interpreter::Interpreter &interpreter)
        {
            interpreter.installSegment5(Compiler::Network::opcodeNetworkJoin, new OpNetworkJoin);
            interpreter.installSegment5(Compiler::Network::opcodeNetworkCreate, new OpNetworkCreate);
            interpreter.installSegment5(Compiler::Network::opcodeNetworkClose, new OpNetworkClose);
        }
    }
}