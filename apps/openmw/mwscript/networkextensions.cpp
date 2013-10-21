#include "networkextensions.hpp"

#include <boost/lexical_cast.hpp>

#include <components/interpreter/interpreter.hpp>
#include <components/interpreter/opcodes.hpp>
#include <components/compiler/opcodes.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"

#include "../mwmechanics/aipuppet.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "../mwnetwork/networkimp.hpp"

#include "interpretercontext.hpp"
#include "ref.hpp"

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
                MWBase::Environment::get().getNetwork()->connect(address, password);
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
                MWBase::Environment::get().getNetwork()->openServer(port, protocol);
                runtime.getContext().report("Server open.");
            }
        };

        class OpNetworkClose : public Interpreter::Opcode0
        {
        public:

            virtual void execute (Interpreter::Runtime& runtime)
            {
                MWBase::Environment::get().getNetwork()->close();
                runtime.getContext().report("Server Closed");
            }
        };

        template<class R>
        class OpAiPuppet : public Interpreter::Opcode0
        {
            public:

                virtual void execute (Interpreter::Runtime& runtime)
                {
                    MWWorld::Ptr ptr = R()(runtime);

                    std::string secretPhrase = runtime.getStringLiteral (runtime[0].mInteger);
                    runtime.pop();

                    MWBase::World *world = MWBase::Environment::get().getWorld();

                    MWBase::Environment::get().getNetwork()->createPuppet(secretPhrase, ptr);

                    MWMechanics::AiPuppet aiPackage(secretPhrase);
                    MWWorld::Class::get (ptr).getCreatureStats(ptr).getAiSequence().stack(aiPackage);

                    runtime.getContext().report("Created a Networked Puppet.");
                }
        };

        void installOpcodes(Interpreter::Interpreter &interpreter)
        {
            interpreter.installSegment5(Compiler::Network::opcodeNetworkJoin, new OpNetworkJoin);
            interpreter.installSegment5(Compiler::Network::opcodeNetworkCreate, new OpNetworkCreate);
            interpreter.installSegment5(Compiler::Network::opcodeNetworkClose, new OpNetworkClose);

            interpreter.installSegment5(Compiler::Network::opcodeAiPuppet, new OpAiPuppet<ImplicitRef>);
            interpreter.installSegment5(Compiler::Network::opcodeAiPuppetExplicit, new OpAiPuppet<ExplicitRef>);
        }
    }
}