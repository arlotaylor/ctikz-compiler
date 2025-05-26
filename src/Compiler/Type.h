#pragma once
#include "Assertion.h"
#include "Lexer.h"
#include <variant>
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
    std::vector<T>& vec;
    int begin;

    inline const T& operator[](int i) { return vec[begin + i]; }
    inline VectorView SubView(int nb) { return { vec, begin + nb }; }

    inline VectorView(std::vector<T>& v, int b) : vec(v), begin(b) {}
    inline VectorView(const VectorView<T>& other) : vec(other.vec), begin(other.begin) {}
    inline VectorView<T>& operator=(const VectorView<T>& other) { vec = other.vec; begin = other.begin; return *this; }
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

struct ErrorOutput
{
    std::string msg; TextPosition pos;
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

    UnionType(std::vector<HeapAlloc<Type>> v);
};

struct OverloadType
{
    std::vector<HeapAlloc<Type>> values;

    OverloadType(std::vector<HeapAlloc<Type>> v);
};

struct RecordType
{
    std::vector<HeapAlloc<Type>> values;

    RecordType(std::vector<HeapAlloc<Type>> v);
};

struct LambdaType
{
    HeapAlloc<Type> arg;
    HeapAlloc<Type> ret;
    bool isTemplate;
    std::vector<HeapAlloc<Type>> instantiatedArgs;
    std::vector<VectorView<Token>> definitions;
    // TODO: track the return types of every combination, then add that stuff to CheckCast

    void CheckArgDef(int instArg, int definition, std::vector<ErrorOutput>& errors);
    void AddInstArgs(Type a, std::vector<ErrorOutput>& errors);
    void AddDefinition(VectorView<Token> tokens, std::vector<ErrorOutput>& errors);

    LambdaType(HeapAlloc<Type> a, HeapAlloc<Type> r);
};

bool operator==(const Type& a, const Type& b);
bool operator!=(const Type& a, const Type& b);

bool CheckCast(Type from, Type to);


struct ParsingContext
{
    std::vector<std::pair<std::string, Type>> varStack;
    std::map<std::string, Type> typedefs = { { "int", AtomicType::Integer }, { "double", AtomicType::Double }, { "string", AtomicType::String }, { "bool", AtomicType::Boolean } };
    std::vector<ErrorOutput> errors;
};

enum class TypeParsingPrecedence
{
    Record = 0, Union, Overload, Lambda, Bracket, Atomic
};

bool ParseType(VectorView<Token> tokens, ParsingContext& ctx, Type& outType, int& tokensConsumed, TypeParsingPrecedence tp = TypeParsingPrecedence::Bracket);

std::string TypeToString(Type t);

