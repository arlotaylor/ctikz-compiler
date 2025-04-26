#define NOBPP_IMPLEMENTATION
#include "vendor/nobpp.hpp"

int main(int argc, char** argv)
{
    nob::Init nobInit(argc, argv, __FILE__);

    std::filesystem::path root = std::filesystem::current_path();

    nob::DefaultCompileCommand = nob::DefaultCompileCommand + nob::IncludeDirectory{ root / "src" } + nob::CompilerFlag::CPPVersion17 + nob::CompilerFlag::OptimizeSpeed;

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
