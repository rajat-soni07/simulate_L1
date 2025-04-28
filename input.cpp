#include <iostream>
#include <fstream>
#include <vector>

std::vector<std::vector<std::vector<long long int>>> input(std::string app) {
    std::vector<std::vector<std::vector<long long int>>> ans;
    for (int i = 0; i<4; i++){
        std::vector<std::vector<long long int>> proc;
        std::string filename = app + "_proc" + std::to_string(i) + ".trace";

        std::ifstream file(filename);
        
        std::string line;
        while(std::getline(file,line)){
            std::vector<long long int> temp;
            temp.push_back((line[0]=='R')?0:1); 
            temp.push_back(std::stoll(line.substr(2), nullptr, 16)); 
            proc.push_back(temp);
        }
        ans.push_back(proc);
    }
    return ans;
}

// int main(){
//     std::string app = "app";
//     auto result = input(app);

//     for (int i = 0; i < result.size(); i++) {
//         std::cout << "Process " << i << ":\n";
//         for (const auto& entry : result[i]) {
//             std::cout << entry[0] << " " << entry[1] << std::endl;
//         }
//         std::cout << "\n";
//     }
//     return 0;
// }