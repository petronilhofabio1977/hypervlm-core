#include <iostream>
#include <filesystem>
int main(int argc, char**argv){
    std::cout<<"DNA Inspect Tool - list files in dna storage (example)"<<std::endl;
    std::string root = (argc>1)?argv[1]:"build/dna_test_repo";
    for(auto &p : std::filesystem::recursive_directory_iterator(root)) {
        std::cout<<p.path().string()<<"\n";
    }
    return 0;
}
