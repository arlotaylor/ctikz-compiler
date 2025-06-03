// #include <cctype>
#include <cstdint>
#include <string>
#include <iostream>  // for logging
#include <vector>  // for reflections
#include <map>
#include <variant>
#include <functional>


#pragma region Compiler


#pragma endregion

#pragma region TypeChecker


enum class InstructionType
{
    WriteValue,
    LoadValue,
    RunFunction,
    PopStack,
    NewStack,
    GotoIf,
    EnterLambda,
    ExitLambda,
};

struct Instruction
{
    InstructionType type;
    int value;  // can be the write location, the value to load, the function to read, or the place to go to, or the number of stack frames to drop
    int value2 = 0;  // can be the relative stack pointer, -1 for global, or the bool isRelative for the goto
    int value3 = 0; // can be the number of bytes to load
};


struct ProgramContext
{
    std::vector<uint8_t> varStack;  // the memory stack of the program
    std::vector<int> stackPtrs;  // the scope starting points

    std::vector<uint8_t> exprStack;  // the current expression stack
};

typedef void(*VoidPtrFunc)(void*);

struct TrackedPointer
{
    void* value;
    VoidPtrFunc deleteFunc;
    bool isStored;

    template<typename T>
    T& Get()
    {
        return *(T*)value;
    }

    void Delete(bool force=false)
    {
        if (value == nullptr) return;

        if (force || !isStored)
        {
            deleteFunc(value);
            value = nullptr;
        }
    }
};

template<typename T> TrackedPointer NewTrackedPointer(const T& val, bool isStored = false)
{
    return { (void*) new T{val}, [](void* v){ delete (T*)v; }, isStored };
}

template<typename T> T Read(uint8_t* arr) { static_assert(std::is_trivially_copyable<T>::value); T out; memcpy(&out, arr, sizeof(T)); return out; }
template<typename T> void Write(uint8_t* arr, const T& val) { static_assert(std::is_trivially_copyable<T>::value); memcpy(arr, &val, sizeof(T)); }


// passing pointers to these functions gives them ownership, those pointers will be deleted.
const std::vector<std::pair<std::pair<int, int>, std::function<void(uint8_t*,uint8_t*)>>> s_BuiltinFuncs = {
    { { 8, 4 }, [](uint8_t* in, uint8_t* out){ Write<int32_t>(out, Read<int32_t>(in) + Read<int32_t>(in+4)); } },  // add two integers
    { { 2 * sizeof(double), sizeof(double) }, [](uint8_t* in, uint8_t* out){ Write<double>(out, Read<double>(in) + Read<double>(in + sizeof(double))); } },  // add two integers
    { { 2 * sizeof(TrackedPointer), sizeof(TrackedPointer) }, [](uint8_t* in, uint8_t* out){ TrackedPointer strA = Read<TrackedPointer>(in); TrackedPointer strB = Read<TrackedPointer>(in + sizeof(TrackedPointer)); Write<TrackedPointer>(out, NewTrackedPointer(strA.Get<std::string>() + strB.Get<std::string>())); strA.Delete(); strB.Delete(); } }, // add two strings
    { { sizeof(TrackedPointer), 0 }, [](uint8_t* in, uint8_t* out){ TrackedPointer strA = Read<TrackedPointer>(in); std::cout << strA.Get<std::string>(); strA.Delete(); } },  // print a string
    { { 4, sizeof(double) }, [](uint8_t* in, uint8_t* out){ Write<double>(out, (double)Read<int32_t>(in)); } },  // int to float
    { { sizeof(double), 4 }, [](uint8_t* in, uint8_t* out){ Write<int32_t>(out, (int32_t)Read<double>(in)); } },  // float to int
    { { 4, sizeof(TrackedPointer) }, [](uint8_t* in, uint8_t* out){ Write<TrackedPointer>(out, NewTrackedPointer<std::string>(std::to_string(Read<int32_t>(in)))); } },  // int to string
    { { sizeof(double), sizeof(TrackedPointer) }, [](uint8_t* in, uint8_t* out){  Write<TrackedPointer>(out, NewTrackedPointer<std::string>(std::to_string(Read<double>(in)))); } },  // float to string
    { { 4, 4 }, [](uint8_t* in, uint8_t* out){  } },
    { { 4, 4 }, [](uint8_t* in, uint8_t* out){  } },
    { { 4, 4 }, [](uint8_t* in, uint8_t* out){  } },
};

void RunBuiltin(ProgramContext& ctx, int index)
{
    uint8_t* in = &ctx.exprStack[ctx.exprStack.size() - s_BuiltinFuncs[index].first.first];
    uint8_t* out = new uint8_t[s_BuiltinFuncs[index].first.second];
    s_BuiltinFuncs[index].second(in, out);
    for (int i = 0; i < s_BuiltinFuncs[index].first.first; i++)
    {
        ctx.exprStack.pop_back();
    }
    for (int i = 0; i < s_BuiltinFuncs[index].first.second; i++)
    {
        ctx.exprStack.push_back(out[i]);
    }
    delete[] out;
}

std::string AddEscapes(std::string str)
{
    std::string ret = "";
    for (int i = 0; i < str.size(); i++)
    {
        size_t pos = std::string("\a\b\f\n\r\t\v\\\'\"").find(str[i]);
        if (pos == std::string::npos)
        {
            ret += str[i];
        }
        else
        {
            ret += "\\";
            ret += std::string("abfnrtv\\\'\"")[pos];
        }
    }
    return "\"" + ret + "\"";
}

std::string RemoveEscapes(std::string str)
{
    if (str.size() < 2 || str[0] != '"' || str[str.size()-1] != '"')
    {
        std::cout << "Error: invalid string passed...";
        return "";
    }

    str = str.substr(1, str.size() - 2);
    std::string ret = "";
    for (int i = 0; i < str.size(); i++)
    {
        if (str[i] == '\\')
        {
            i++; if (i >= str.size()) break;
            ret += std::string("\a\b\f\n\r\t\v\\\'\"")[std::string("abfnrtv\\\'\"").find(i)];
        }
        else
        {
            ret += str[i];
        }
    }

    return ret;
}

int GetTypeSize(LanguageType type)
{
    if (std::holds_alternative<AtomicType>(type))
    {
        switch (std::get<AtomicType>(type))
        {
            case AtomicType::Error: return 0; break;
            case AtomicType::Void: return 0; break;
            case AtomicType::EndSum: return 0; break;
            case AtomicType::Template: return 0; break;
            case AtomicType::Int: return 4; break;
            case AtomicType::Double: return sizeof(double); break;
            case AtomicType::String: return sizeof(TrackedPointer); break;
            case AtomicType::Drawable: return 0; break;  // TODO: figure out what to put here after Drawable class is implemented
            default: break;
        }
    }
    else if (std::holds_alternative<UnionType>(type))
    {
        UnionType& ut = std::get<UnionType>(type);
        return std::max(4 + GetTypeSize(*ut.a), (std::holds_alternative<UnionType>(*ut.b) ? 0 : 4) + GetTypeSize(*ut.b));
    }
    else if (std::holds_alternative<SumType>(type))
    {
        SumType& st = std::get<SumType>(type);
        return GetTypeSize(*st.a) + GetTypeSize(*st.b);
    }
    else if (std::holds_alternative<LambdaType>(type))
    {
        return 4;
    }
    else
    {
        return 0;
    }
}

int SearchVars(const std::vector<VarSignature>& vars, const VarSignature& elem, CheckCastOut* out = nullptr)
{
    for (int i = 0; i < vars.size(); i++)
    {
        if (vars[i].name != elem.name) continue;
        CheckCastOut o = CheckCast(vars[i].type, elem.type);
        if (o != CheckCastOut::Failure)
        {
            if (out != nullptr) *out = o;
            return i;
        }
    }

    if (out != nullptr) *out = CheckCastOut::Failure;
    return -1;
}

std::pair<LanguageType, LinkedList<Instruction>> GenerateBytecode(const std::string& str, ASTGNode* node, int nesting, std::map<std::string,std::pair<std::pair<int,int>, LanguageType>>& names, LinkedList<Instruction>& header, std::vector<uint8_t>& globals)
{
    switch (node->type)
    {
        case GrammarType::Integer:
            {
                for (int i = 0; i < 4; i++) globals.push_back(0);
                Write<int32_t>(&globals[globals.size() - 4], std::stoi(node->range.GetSub(str)));
                return { AtomicType::Int, { header.GetContainer(), Instruction{ InstructionType::LoadValue, (int)globals.size() - 4, -1, 4 } } };
            }
            break;
        case GrammarType::Decimal:
            {
                for (int i = 0; i < 8; i++) globals.push_back(0);
                Write<double>(&globals[globals.size() - sizeof(double)], std::stod(node->range.GetSub(str)));
                return { AtomicType::Double, { header.GetContainer(), Instruction{ InstructionType::LoadValue, (int)globals.size() - (int)sizeof(double), -1, (int)sizeof(double) } } };
            }
            break;
        case GrammarType::StringLit:
            {
                for (int i = 0; i < sizeof(TrackedPointer); i++) globals.push_back(0);
                Write<TrackedPointer>(&globals[globals.size() - sizeof(TrackedPointer)], NewTrackedPointer<std::string>(RemoveEscapes(node->range.GetSub(str)), true));
                return { AtomicType::String, { header.GetContainer(), Instruction{ InstructionType::LoadValue, (int)globals.size() - (int)sizeof(TrackedPointer), -1, sizeof(TrackedPointer) } } };
            }
            break;
        case GrammarType::Identifier:
            {
                std::string name = node->range.GetSub(str);
                if (!names.count(name))
                {
                    std::cout << "Error: unrecognized identifier.";
                    return { AtomicType::Error, { header.GetContainer() } };
                }

                return { names[name].second, { header.GetContainer(), Instruction{ InstructionType::LoadValue, names[name].first.first, nesting - names[name].first.second, GetTypeSize(names[name].second) } } };
            }
            break;
        case GrammarType::Arguments:
            {
                // assume not in a lambda, that should be handled in Lambda
                LanguageType ty = AtomicType::EndSum;
                LinkedList<Instruction> ll = { header.GetContainer() };

                ASTGNode* child = node->child;
                if (child == nullptr) return { ty, ll };
                while (child->right != nullptr) child = child->right;

                while (child != nullptr)
                {
                    std::pair<LanguageType, LinkedList<Instruction>> o = GenerateBytecode(str, child, nesting, names, header, globals);
                    ty = SumType(o.first, ty);
                    ll = o.second + ll;
                    child = child->left;
                }

                return { ty, ll };
            }
            break;
        case GrammarType::FunctionCall:
            {
                std::pair<LanguageType, LinkedList<Instruction>> idType = GenerateBytecode(str, node->child, nesting, names, header, globals);
                std::pair<LanguageType, LinkedList<Instruction>> argType = GenerateBytecode(str, node->child->right, nesting, names, header, globals);
                LanguageType lt = idType.first;
                LanguageType& ty = lt;
                LanguageType res = AtomicType::Error;
                CheckCastOut cco = CheckCastOut::Failure;
                bool shouldError = true;

                while (true)
                {
                    if (std::holds_alternative<UnionType>(lt))
                    {
                        ty = *std::get<UnionType>(lt).a;
                    }
                    else
                    {
                        ty = lt;
                    }

                    if (std::holds_alternative<LambdaType>(ty))
                    {
                        CheckCastOut co = CheckCast(ty, LambdaType{ argType.first, *std::get<LambdaType>(ty).returnType });
                        if (co == cco) shouldError = true;
                        else if (co > cco)
                        {
                            shouldError = false;
                            res = ty;
                            cco = co;
                        }
                    }
                    if (!std::holds_alternative<UnionType>(lt)) break;
                    lt = *std::get<UnionType>(lt).b;
                }

                if (shouldError)
                {
                    std::cout << "Error: Too many matching casts in union.\n";
                    return { AtomicType::Error, { header.GetContainer() } };
                }

                switch (cco)
                {
                    case CheckCastOut::Failure:
                    {
                        std::cout << "Error: No matching function signature.\n";
                        return { AtomicType::Error, { header.GetContainer() } };
                    }
                    break;
                    case CheckCastOut::Template:
                    {

                    }
                    break;
                    case CheckCastOut::Cast:
                    {

                    }
                    break;
                    case CheckCastOut::Success:
                    {
                        return { std::get<LambdaType>(res).returnType,  }
                    }
                    break;
                    default: break;
                }
            }
            break;
        case GrammarType::ParenExpr:
            {
            }
            break;
        case GrammarType::ExpExpr:
            {
            }
            break;
        case GrammarType::MulExpr:
            {
            }
            break;
        case GrammarType::DivExpr:
            {
            }
            break;
        case GrammarType::ModExpr:
            {
            }
            break;
        case GrammarType::AddExpr:
            {
            }
            break;
        case GrammarType::SubExpr:
            {
            }
            break;
        case GrammarType::EqExpr:
            {
            }
            break;
        case GrammarType::NeqExpr:
            {
            }
            break;// no bitwise operations
        case GrammarType::AndExpr:
            {
            }
            break;
        case GrammarType::OrExpr:
            {
            }
            break;
        case GrammarType::VarAssignment:
            {
            }
            break;
        case GrammarType::Expression:
            {
            }
            break;// set var first
        case GrammarType::Statement:
            {
            }
            break;
        case GrammarType::Block:
            {
            }
            break;
        case GrammarType::Scope:
            {
            }
            break;
        case GrammarType::IfStatement:
            {
            }
            break;
        case GrammarType::WhileStatement:
            {
            }
        break;
        case GrammarType::ForStatement:
            {
            }
        break;
        case GrammarType::Lambda:
            {
            }
        break;
        default: break;
    }
}



class PointerManager
{
    std::map<int, TrackedPointer> stackToPtr;
public:
    bool IsManaged(int stackPtr)
    {
        return !!stackToPtr.count(stackPtr);
    }

    void Add(int stackPtr, TrackedPointer ptr)
    {
        stackToPtr.insert({ stackPtr, ptr });
    }

    void Remove(int stackPtr)
    {
        if (!IsManaged(stackPtr)) return;

        stackToPtr[stackPtr].Delete(true);
        stackToPtr.erase(stackPtr);
    }
};

// we need to wrap pointers so that the builtins only delete the ones from the expression stack, and the others are cleared when the relevent var stack is popped
// this can be achieved by adding a bool to the pointers, which is default constructed to be false. Then when 'delete' is called on the object, it only deletes if
// the bool is false, or if a force param is set to true. Then these are tracked by a pointer manager, which keeps copies of the permanent pointers, associated to
// stack positions. Then when that stack position is popped, this object can be properly deleted. When a WriteValue is executed, the third parameter can be set to
// -1 to indicate that this is a pointer that should be tracked. The initial pointers do not have to be tracked, because they will (hopefully) never be deleted.
// Unfortunately we will probably have to include a function pointer in each of these objects, because otherwise we won't be able to call the right destructor.

void RunProgram(LinkedList<Instruction> inst, std::vector<uint8_t> initials)
{
    ProgramContext ctx = { initials, { (int)initials.size() }, {} };
    PointerManager ptrTracker;

    for (int instPtr = 0; instPtr < inst.GetSize(); instPtr++)
    {
        const Instruction& instruction = inst[instPtr];
        switch (instruction.type)
        {
        case InstructionType::WriteValue:
        {
            int loc = (instruction.value2 == -1 ? 0 : ctx.stackPtrs[ctx.stackPtrs.size() - 1 - instruction.value2]) + instruction.value;
            // copy the data
            int writeLength = instruction.value3 == -1 ? sizeof(TrackedPointer) : instruction.value3;
            for (int i = 0; i < writeLength; i++)
            {
                if (loc + i >= ctx.varStack.size())
                {
                    while (loc + i > ctx.varStack.size()) ctx.varStack.push_back(0);

                    ctx.varStack.push_back(ctx.exprStack[ctx.exprStack.size() - writeLength + i]);
                }
                else
                {
                    ctx.varStack[loc + i] = ctx.exprStack[ctx.exprStack.size() - writeLength + i];
                }
            }
            // delete the data
            for (int i = 0; i < writeLength; i++)
            {
                ctx.exprStack.pop_back();
            }

            // if it's a TrackedPointer, track it!
            if (instruction.value3 == -1) ptrTracker.Add(loc, Read<TrackedPointer>(&ctx.varStack[loc]));
        }
        break;
        case InstructionType::LoadValue:
        {
            int loc = (instruction.value2 == -1 ? 0 : ctx.stackPtrs[ctx.stackPtrs.size() - 1 - instruction.value2]) + instruction.value;
            for (int i = 0; i < instruction.value3; i++)
            {
                ctx.exprStack.push_back(ctx.varStack[loc + i]);
            }
        }
        break;
        case InstructionType::RunFunction:
            RunBuiltin(ctx, instruction.value);
        break;
        case InstructionType::PopStack:
            for (int i = 0; i < instruction.value; i++)
            {
                if (ctx.stackPtrs.size() <= 1) return;
                while (ctx.varStack.size() > ctx.stackPtrs.back())
                {
                    ctx.varStack.pop_back();
                    ptrTracker.Remove(ctx.varStack.size());
                }
                ctx.stackPtrs.pop_back();
            }
        break;
        case InstructionType::NewStack:
            ctx.stackPtrs.push_back(ctx.varStack.size());
        break;
        case InstructionType::GotoIf:
            if (ctx.exprStack.back())
            {
                instPtr = (instruction.value2 ? instPtr : 0) + instruction.value - 1;
            }
            ctx.exprStack.pop_back();
        break;
        case InstructionType::EnterLambda:
            ctx.stackPtrs.push_back(ctx.varStack.size());
            for (int i = 0; i < 4; i++) ctx.varStack.push_back(0);
            *(int*)&ctx.varStack[ctx.varStack.size() - 4] = instPtr + 1;
            instPtr = instruction.value - 1;
        break;
        case InstructionType::ExitLambda:
            for (int i = 0; i < instruction.value; i++)
            {
                if (ctx.stackPtrs.size() <= 1) return;
                ctx.stackPtrs.pop_back();
            }
            instPtr = (*(int*)&ctx.varStack[ctx.stackPtrs.back()]) - 1;
            while (ctx.varStack.size() > ctx.stackPtrs.back())
            {
                ctx.varStack.pop_back();
                ptrTracker.Remove(ctx.varStack.size());
            }
            ctx.stackPtrs.pop_back();
        break;
        default: break;
        }
    }
}






LanguageType TypeCheck(const std::string& str, ASTGNode* node, std::vector<VarSignature>& vars)
{
    if (node->type == GrammarType::Integer)
    {
        return AtomicType::Int;
    }
    else if (node->type == GrammarType::Decimal)
    {
        return AtomicType::Double;
    }
    else if (node->type == GrammarType::StringLit)
    {
        return AtomicType::String;
    }
    else if (node->type == GrammarType::Identifier)
    {
        int loc = SearchVars(vars, {str.substr(node->range.begin, node->range.end - node->range.begin)});
        if (loc == -1)
        {
            return AtomicType::Error;
        }
        else
        {
            return vars[loc].type;
        }
    }
    else if (node->type == GrammarType::FunctionCall)
    {
        if (node->left->type != GrammarType::Identifier) std::cout << "Error: Function call with invalid identifier???\n";
        LanguageType lt = TypeCheck(str, node->right, vars);

        CheckCastOut locType = CheckCastOut::Failure;
        int loc = SearchVars(vars, {str.substr(node->left->range.begin, node->left->range.end - node->left->range.begin), LambdaType{ lt, AtomicType::Template } }, &locType);
        if (loc == -1)
        {
            return AtomicType::Error;
        }

        LanguageType lt2 = vars[loc].type;
        if (!std::holds_alternative<LambdaType>(lt2))
        {
            return AtomicType::Error;  // shouldn't ever run
        }
        else
        {
            const LambdaType& lambdaType = std::get<LambdaType>(lt2);
            LanguageType argT = TypeCheck(str, node->right, vars);
            if (!CheckCast(*lambdaType.argumentType, argT))
            {
                return AtomicType::Error;
            }
        }
    }

}

#pragma endregion



int main()
{
    std::cout << "Enter mathematical expression: ";
    std::string expr; std::getline(std::cin, expr);
    std::cout << "Recieved " << expr << ".\n";

    ASTGNode* program = Parse<GrammarType::Block>(RemoveStrings(expr), 0);
    if (program != nullptr)
    {
        ASTGNode* ptr = program;

        std::cout << ASTGNodeToString(ptr, expr) << std::endl;
    }
    else
    {
        std::cout << "Syntax Error!\n";
    }
}


