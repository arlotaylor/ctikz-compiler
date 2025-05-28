#define NOBPP_IMPLEMENTATION
#include "vendor/nobpp.hpp"

int main(int argc, char** argv)
{
    nob::Init nobInit(argc, argv, __FILE__);

    std::filesystem::path root = std::filesystem::current_path();

    nob::DefaultCompileCommand = nob::DefaultCompileCommand + nob::IncludeDirectory{ root / "src" } + nob::CompilerFlag::CPPVersion17 + nob::CompilerFlag::OptimizeSpeed;

    for (std::string& i : nob::OtherCLArguments)
    {
        if (i == "-test")
        {
            nob::DefaultCompileCommand = nob::DefaultCompileCommand + nob::MacroDefinition{ "RUN_TESTS", "1" };
            nob::CLFlags.set(nob::CLArgument::Clean);
            break;
        }
        else if (i == "-maketest")
        {
            nob::DefaultCompileCommand = nob::DefaultCompileCommand + nob::MacroDefinition{ "CREATE_TESTS", "1" };
            nob::CLFlags.set(nob::CLArgument::Clean);
            break;
        }
        else
        {
            nob::Log("Argument ignored: " + i);
        }
    }

    if (nob::CLFlags[nob::CLArgument::Clean])
    {
        nob::Log("Deleting bin...\n", nob::LogType::Info);
        std::filesystem::remove_all(root / "bin");
    }

    std::filesystem::create_directories(root / "bin" / "int");
    nob::CompileDirectory(root / "src", root / "bin" / "int");
    nob::LinkDirectory(root / "bin" / "int", root / "bin" / (root.filename().string() + ".exe"));

    return 0;
}
