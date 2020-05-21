#pragma once

/**
 * @file basic-callback.h
 * @author karurochari
 * @brief 
 * @version 0.1
 * @date 2020-04-27
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <nlohmann/json.hpp>
#include <cpr/cpr.h>
#include <cstdlib>

#include "string-exception.h"


struct basic_callback{
    friend void to_json(nlohmann::json& i, const basic_callback& m){

    }

    friend void from_json(const nlohmann::json& config, basic_callback& m){
        {
            auto it=config.find("url");
            if(it!=config.end() && it->is_string()){
                m.url.value()=*it;
            }
            else if(it!=config.end())_type_mismatch("url","string");
            else;
        }
        {
            auto it=config.find("script");
            if(it!=config.end() && it->is_string()){
                m.script.value()=*it;
            }
            else if(it!=config.end())_type_mismatch("script","string");
            else;
        }
    }

    template<typename T>
    void operator()(const T& i) const{
        if(url.has_value()){
            cpr::Get(cpr::Url{url.value()});
        }

        if(script.has_value()){
            std::system(script.value().c_str());
        }
    }

    private:
        std::optional<std::string> url;
        std::optional<std::string> script;

        static void _type_mismatch(const std::string& field, const std::string& expected){
            throw StringException("TypeMismatchException for ["+field+"] expected ["+expected+"]");
        }
};