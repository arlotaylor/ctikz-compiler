#pragma once
#include "Lexer.h"
#include <variant>
#include <memory>
#include <vector>
#include <map>

// This language's types fall into five categories
// Atomic Types, which are simple data (sometimes in TrackedPointers). This category also includes bookkeeping types like Error, Void and EndSum, as well as Template.
// Unions, which are tagged unions, with one int beforehand to store the tag. These do not have to have different types, as the tag is just an index. These types can be cast to larger unions, or split, with the split operator.
// Sums, which are record types. A sum can be cast to another sum if all elements cast accordingly. A sum type can be indexed into as well.
// Lambdas, which are functions. Multiple arguments are represented as a sum. Can cast to another lambda where the arguments and return type cast accordingly. Can also curry???
// Overloads, which are essentially sums, where each type is unique (up to casts) and can cast to any of its constituents. Can cast to a subset overload.

// Basically the rules are the following, where sums are <a,b>, unions are <a|b>, lambdas are <a->b> and overloads are <a&b>:
// if a casts to A:

template<typename T>
struct VectorView
{
    const std::vector<T>& vec;
    int begin;

    inline const T& operator[](int i) { return vec[begin + i]; }
    inline VectorView SubView(int nb) { return { vec, begin + nb }; }
};

template<typename T>
class HeapAlloc
{
    T* val;
public:
    T& Get() { return *val; }
    const T& Get() const { return *val; }

    HeapAlloc(const T& v)
    {
        val = new T{v};
    }
    HeapAlloc(const HeapAlloc<T>& other)
    {
        val = new T{other.Get()};
    }
    ~HeapAlloc() { delete val; }
    HeapAlloc& operator=(const HeapAlloc& other)
    {
        delete val;
        val = new T{other.Get()};
        return *this;
    }
};

enum class AtomicType
{
    Error, Void, Template,
    Integer, Double, String, Boolean,
};

struct UnionType; struct OverloadType; struct RecordType; struct LambdaType;
typedef std::variant<AtomicType, UnionType, OverloadType, RecordType, LambdaType> Type;

struct UnionType
{
    std::vector<HeapAlloc<Type>> values;
};

struct OverloadType
{
    std::vector<HeapAlloc<Type>> values;
};

struct RecordType
{
    std::vector<HeapAlloc<Type>> values;
};

struct LambdaType
{
    HeapAlloc<Type> arg;
    HeapAlloc<Type> ret;
};

bool operator==(const Type& a, const Type& b);
bool operator!=(const Type& a, const Type& b);


struct ErrorOutput
{
    std::string msg; int line; int column;
};

struct ParsingContext
{
    std::map<std::string, Type> typedefs = { { "int", AtomicType::Integer }, { "double", AtomicType::Double }, { "string", AtomicType::String }, { "bool", AtomicType::Boolean } };
    std::vector<std::pair<std::string, Type>> varStack;
    std::vector<ErrorOutput> errors;
};

enum class TypeParsingPrecedence
{
    Record = 0, Union, Overload, Lambda, Bracket, Atomic
};

bool ParseType(VectorView<Token> tokens, ParsingContext& ctx, Type& outType, int& tokensConsumed, TypeParsingPrecedence tp = TypeParsingPrecedence::Record);

