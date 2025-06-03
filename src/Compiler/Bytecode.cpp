#include "Bytecode.h"

void GenerateBytecode(const Expression& e, InstructionSet& out)
{
    if (std::holds_alternative<LiteralExpression>(e))
    {
        switch (std::get<AtomicType>(std::get<LiteralExpression>(e).type))
        {
        case AtomicType::Integer:
            out.body.push_back(PushLiteralInstruction{ AtomicType::Integer, AtomicInstance{ std::stoi(std::get<LiteralExpression>(e).vec[0].value) } });
            break;
        case AtomicType::Double:
            out.body.push_back(PushLiteralInstruction{ AtomicType::Double, AtomicInstance{ std::stod(std::get<LiteralExpression>(e).vec[0].value) } });
            break;
        case AtomicType::String:
            out.body.push_back(PushLiteralInstruction{ AtomicType::String, AtomicInstance{ std::get<LiteralExpression>(e).vec[0].value } });
            break;
        default: break;
        }
    }
    else if (std::holds_alternative<VariableExpression>(e))
    {
        out.body.push_back(PushVariableInstruction{ std::get<VariableExpression>(e).stackIndex });
    }
    else if (std::holds_alternative<LambdaExpression>(e))
    {
        
    }
    else if (std::holds_alternative<MultiExpression>(e))
    {
    }
    else if (std::holds_alternative<BinaryExpression>(e))
    {
    }
    else  // assumed std::holds_alternative<UnaryExpression>(e)
    {
    }
}

void GenerateBytecode(const Statement& s, InstructionSet& out)
{

}

std::vector<Instruction> FuseInstructionSet(const InstructionSet& out)
{
    std::vector<Instruction> ret = { GotoIfInstruction{ out.header.size() + 1, GotoIfType::Static } };
    ret.insert(ret.end(), out.header.begin(), out.header.end());
    ret.insert(ret.end(), out.body.begin(), out.body.end());
    return ret;
}

void RunBytecode(const std::vector<Instruction>& code)
{

}
