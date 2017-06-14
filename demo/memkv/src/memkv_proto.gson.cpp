#include "memkv_proto.h"
#include "memkv_proto.gson.h"
#include "acl_cpp/serialize/gson_helper.ipp"
namespace acl
{
    acl::json_node& gson(acl::json &$json, const del_req &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.key))
            $node.add_null("key");
        else
            $node.add_text("key", acl::get_value($obj.key));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const del_req *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const del_req &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, del_req &$obj)
    {
        acl::json_node *key = $node["key"];
        std::pair<bool, std::string> $result;

        if(!key ||!($result = gson(*key, &$obj.key), $result.first))
            return std::make_pair(false, "required [del_req.key] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, del_req *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, del_req &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const del_resp &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_text("status", acl::get_value($obj.status));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const del_resp *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const del_resp &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, del_resp &$obj)
    {
        acl::json_node *status = $node["status"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [del_resp.status] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, del_resp *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, del_resp &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const exist_req &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.key))
            $node.add_null("key");
        else
            $node.add_text("key", acl::get_value($obj.key));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const exist_req *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const exist_req &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, exist_req &$obj)
    {
        acl::json_node *key = $node["key"];
        std::pair<bool, std::string> $result;

        if(!key ||!($result = gson(*key, &$obj.key), $result.first))
            return std::make_pair(false, "required [exist_req.key] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, exist_req *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, exist_req &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const exist_resp &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_text("status", acl::get_value($obj.status));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const exist_resp *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const exist_resp &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, exist_resp &$obj)
    {
        acl::json_node *status = $node["status"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [exist_resp.status] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, exist_resp *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, exist_resp &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const get_req &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.key))
            $node.add_null("key");
        else
            $node.add_text("key", acl::get_value($obj.key));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const get_req *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const get_req &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, get_req &$obj)
    {
        acl::json_node *key = $node["key"];
        std::pair<bool, std::string> $result;

        if(!key ||!($result = gson(*key, &$obj.key), $result.first))
            return std::make_pair(false, "required [get_req.key] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, get_req *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, get_req &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const get_resp &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_text("status", acl::get_value($obj.status));

        if (check_nullptr($obj.value))
            $node.add_null("value");
        else
            $node.add_text("value", acl::get_value($obj.value));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const get_resp *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const get_resp &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, get_resp &$obj)
    {
        acl::json_node *status = $node["status"];
        acl::json_node *value = $node["value"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [get_resp.status] failed:{"+$result.second+"}");
     
        if(!value ||!($result = gson(*value, &$obj.value), $result.first))
            return std::make_pair(false, "required [get_resp.value] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, get_resp *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, get_resp &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const set_req &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.key))
            $node.add_null("key");
        else
            $node.add_text("key", acl::get_value($obj.key));

        if (check_nullptr($obj.value))
            $node.add_null("value");
        else
            $node.add_text("value", acl::get_value($obj.value));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const set_req *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const set_req &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, set_req &$obj)
    {
        acl::json_node *key = $node["key"];
        acl::json_node *value = $node["value"];
        std::pair<bool, std::string> $result;

        if(!key ||!($result = gson(*key, &$obj.key), $result.first))
            return std::make_pair(false, "required [set_req.key] failed:{"+$result.second+"}");
     
        if(!value ||!($result = gson(*value, &$obj.value), $result.first))
            return std::make_pair(false, "required [set_req.value] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, set_req *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, set_req &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


    acl::json_node& gson(acl::json &$json, const set_resp &$obj)
    {
        acl::json_node &$node = $json.create_node();

        if (check_nullptr($obj.status))
            $node.add_null("status");
        else
            $node.add_text("status", acl::get_value($obj.status));


        return $node;
    }
    
    acl::json_node& gson(acl::json &$json, const set_resp *$obj)
    {
        return gson ($json, *$obj);
    }


    acl::string gson(const set_resp &$obj)
    {
        acl::json $json;
        acl::json_node &$node = acl::gson ($json, $obj);
        return $node.to_string ();
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, set_resp &$obj)
    {
        acl::json_node *status = $node["status"];
        std::pair<bool, std::string> $result;

        if(!status ||!($result = gson(*status, &$obj.status), $result.first))
            return std::make_pair(false, "required [set_resp.status] failed:{"+$result.second+"}");
     
        return std::make_pair(true,"");
    }


    std::pair<bool,std::string> gson(acl::json_node &$node, set_resp *$obj)
    {
        return gson($node, *$obj);
    }


     std::pair<bool,std::string> gson(const acl::string &$str, set_resp &$obj)
    {
        acl::json _json;
        _json.update($str.c_str());
        if (!_json.finish())
        {
            return std::make_pair(false, "json not finish error");
        }
        return gson(_json.get_root(), $obj);
    }


}///end of acl.
