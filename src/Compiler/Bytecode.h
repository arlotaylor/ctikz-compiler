#pragma once
#include "Type.h"
#include "Parser.h"

struct TypeInstance;

struct AtomicInstance
{
    std::variant<int, double, std::string> val;
};

struct RecordInstance
{
    std::vector<HeapAlloc<TypeInstance>> vals;
};

struct OverloadInstance
{
    std::vector<HeapAlloc<TypeInstance>> vals;
};

struct LambdaInstance
{
    VectorView loc;
};

struct UnionInstance
{
    int ty;
    HeapAlloc<TypeInstance> val;
};

struct TypeInstance
{
    Type type;
    std::variant<AtomicInstance, RecordInstance, OverloadInstance, LambdaInstance, UnionInstance> val;
};



struct PushLiteralInstruction
{
    TypeInstance val;  // TODO: extract this to an index in an array
};

struct PushVariableInstruction
{
    int vStackIndex;
};

struct RunBuiltinInstruction
{
    int id;
};

struct WriteStackInstruction
{
    int index;
};

enum class GotoIfType
{
    Static, LocationStatic, Dynamic, RelativeStatic, Relative,
};

struct GotoIfInstruction
{
    int pos;
    GotoIfType ty;
};

struct CastInstruction
{
    Type from;
    Type to;
};

enum class CombineType
{
    Union, Record, Overload
}
struct CombineInstruction
{
    int count;
};

typedef std::variant<PushLiteralInstruction, PushVariableInstruction, RunBuiltinInstruction, WriteStackInstruction, GotoIfInstruction, CastInstruction, CombineInstruction> Instruction;

struct InstructionSet
{
    std::vector<Instruction> body;
    std::vector<Instruction> header;  // contains lambda code
};

void GenerateBytecode(const Statement& s, InstructionSet& out);
std::vector<Instruction> FuseInstructionSet(const InstructionSet& out);

void RunBytecode(const std::vector<Instruction>& code);
