#include "codegen/llvm/llvm_context.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <fstream>

namespace codegen {

LLVMContext::LLVMContext(const std::string& moduleName)
    : context_(std::make_unique<llvm::LLVMContext>()) {
    createNewModule(moduleName);
}

LLVMContext::~LLVMContext() = default;

void LLVMContext::createNewModule(const std::string& moduleName) {
    module_ = std::make_unique<llvm::Module>(moduleName, *context_);
    builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
}

void LLVMContext::dumpModule() const {
    module_->print(llvm::errs(), nullptr);
}

bool LLVMContext::writeModuleToFile(const std::string& filename) const {
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC);
    
    if (EC) {
        return false;
    }
    
    module_->print(dest, nullptr);
    return true;
}

std::string LLVMContext::getModuleIR() const {
    std::string IR;
    llvm::raw_string_ostream OS(IR);
    module_->print(OS, nullptr);
    return OS.str();
}

} // namespace codegen