
#include "instance_reader.hpp"

namespace tspd::instance_reader {
    string InstanceReader::getValue(const string& line) {
        size_t pos = line.find(':');
    
        if (pos == std::string::npos) {
            return "";
        }
        
        // Pega tudo após o primeiro ':'
        std::string strval = line.substr(pos + 1);
        
        // Remove espaços/tabs/quebras de linha do início e do fim
        size_t start = strval.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return ""; // Caso a string após o ':' só tenha espaços
        
        size_t end = strval.find_last_not_of(" \t\r\n");
        
        return strval.substr(start, end - start + 1);
    }

    void InstanceReader::parseStrValue(const string& line, string& var){
        if (var != "")
            var += "\t";
        var += getValue(line);
    }

    int InstanceReader::parseDimension(const string& line){
        return stoi(getValue(line));
    }

    node::Node InstanceReader::parseNode(const string& line){
        stringstream stream_line(line);
        int id = 0;
        double x = 0.0, y = 0.0;

        stream_line >> id >> x >> y;
        return node::Node(id, x, y);
    }

    void InstanceReader::readNodeCoordSector(ifstream& file, vector<node::Node>& node){
        string line = "";
        while(getline(file, line)){
            if (line != "EOF" && !file.eof() && line != "")
                node.push_back(parseNode(line));
        }
    }

    TSPD InstanceReader::readInstance(const string& filePath) {

        string text;
        ifstream MyReadFile(filePath);

        string name = "",
            comment = "", 
            type = "", 
            edge_weight_type = "";
        int dimension = 0;
        vector<node::Node> node = {};

        if (MyReadFile.is_open()) {
            string line;
            while (getline(MyReadFile, line)) {
                if(line.find("NAME") != string::npos){
                    parseStrValue(line, name);
                    
                } else if (line.find("COMMENT") != string::npos){
                    parseStrValue(line, comment);
                    
                } else if (line.find("DIMENSION") != string::npos){
                    dimension = parseDimension(line);
                
                } else if (line.find("EDGE_WEIGHT_TYPE") != string::npos){
                    parseStrValue(line, edge_weight_type);
                    
                } else if (line.find("NODE_COORD_SECTION") != string::npos){
                    readNodeCoordSector(MyReadFile, node);
                }
                else if (line.find("TYPE") != string::npos){
                    parseStrValue(line, type);
                } 

            }
            MyReadFile.close();
        }

        return TSPD(name, comment, type, dimension, edge_weight_type, node);
    }
}