//
// Created by mwo on 2/06/16.
//

#ifndef JSONRPC_TEST_BTCMARKETS_H_H
#define JSONRPC_TEST_BTCMARKETS_H_H

#include "cpr/cpr.h"
#include "json.hpp"

#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/buffer.h>

#include <iostream>
#include <chrono>
#include <vector>

namespace btcm
{

using namespace std;
using namespace nlohmann;


/**
 * source: http://codereview.stackexchange.com/a/78539
 */
std::string
to_hex(const unsigned char *data, int len)
{
    constexpr char hexmap[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                               '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    std::string s(len * 2, ' ');

    for (int i = 0; i < len; ++i)
    {
        s[2 * i]     = hexmap[(data[i] & 0xF0) >> 4];
        s[2 * i + 1] = hexmap[data[i] & 0x0F];
    }

    return s;
}

template<typename T = std::chrono::seconds>
uint64_t
current_timestamp()
{
    T ms;
    ms = std::chrono::duration_cast<T>(
            std::chrono::system_clock::now().time_since_epoch());

    return ms.count();
}

/**
 * based on http://www.askyb.com/cpp/openssl-hmac-hasing-example-in-cpp/
 */
std::vector<unsigned char>
HMAC_SHA512(const std::string& key, const std::string& data)
{
    unsigned char* digest;
    unsigned int digest_length = 64; // for sha512 length is 64 bytes

    digest = HMAC(EVP_sha512(), key.c_str(),
                  key.length(),
                  reinterpret_cast<unsigned char*>(
                          const_cast<char *>(data.c_str())),
                  data.length(),
                  NULL, NULL);

    std::vector<unsigned char> digest_v(digest, digest + digest_length);

    return digest_v;
}


// the base64 functions
// were adapted from here:
// http://www.adp-gmbh.ch/cpp/common/base64.html

static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";


static inline bool
is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string
base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';

    }

    return ret;

}

std::string
base64_decode(std::string const& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i ==4) {
            for (i = 0; i <4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j <4; j++)
            char_array_4[j] = 0;

        for (j = 0; j <4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }

    return ret;
}


// adapted from monero project:
// https://github.com/monero-project/bitmonero
bool
parse_amount(const std::string& str_amount_,
             uint64_t& amount,
             size_t length = 8)
{
    std::string str_amount = str_amount_;

    size_t point_index = str_amount.find_first_of('.');
    size_t fraction_size;

    if (std::string::npos != point_index)
    {
        fraction_size = str_amount.size() - point_index - 1;
        while (length < fraction_size && '0' == str_amount.back())
        {
            str_amount.erase(str_amount.size() - 1, 1);
            --fraction_size;
        }

        if (length < fraction_size)
            return false;

        str_amount.erase(point_index, 1);
    }
    else
    {
        fraction_size = 0;
    }

    if (str_amount.empty())
        return false;

    if (fraction_size < length)
    {
        str_amount.append(length - fraction_size, '0');
    }

    amount = stoull(str_amount);

    return true;
}

uint64_t
parse_amount(const std::string& str_amount_,
             size_t length = 8)
{
    uint64_t v;

    parse_amount(str_amount_, v, length);

    return v;
}


inline uint64_t
double_to_uint(double no)
{
    stringstream ss;

    ss << std::fixed << std::setprecision(8) << no;

    return parse_amount(ss.str());
}

inline double
uint_to_double(uint64_t no)
{
    return static_cast<double>(no) / 1e8;
}

string
json_str_value(string v)
{
    stringstream ss;
    ss << "\"" << v << "\"";
    return ss.str();
}

template<typename T>
string
json_str_value(T v)
{
    stringstream ss;
    ss << v;
    return ss.str();
}


template<typename T>
string
json_pair(string arg_name, T value, string coma="")
{
    return "\""+arg_name + "\"" + ":" + json_str_value(value) + coma;
}


bool
str_replace(string& str, const string& from, const string& to)
{
    size_t start_pos = str.find(from);

    if(start_pos == std::string::npos)
        return false;

    str.replace(start_pos, from.length(), to);

    return true;
}


class BtcMarkets
{
    string base_url {"https://api.btcmarkets.net"};

    // available trading paris: instrument/currency
    vector<string> trading_pairs {"BTC/AUD", "LTC/AUD", "ETH/AUD", 
    							  "ETC/AUD", "ETC/BTC", 	
                                  "LTC/BTC", "ETH/BTC", "DAO/BTC",
                                  "DAO/ETH"};

    string api_key;
    string private_key;

    // base64 decoded version of the private_key
    string decoded_private_key;

    // btcmarkets requires correct timestamps which should be
    // within +/- 30 seconds of the server timestamp
    // Thus if you are in incorrect timezone, your timestamp will different
    // from the timestamp of the server, and an error will occure.
    // If you are using different timezone you can set this timestamp offset
    // variable to account for differences in timezones in seconds.
    int64_t timestamp_offset {0}; // offset in seconds

    int32_t request_timeout {1000}; // timeout in milliseconds

public:


    BtcMarkets(int32_t _request_timeout = 1000,
               int64_t _timestamp_offset = 0)
            : request_timeout {_request_timeout},
              timestamp_offset {_timestamp_offset},
              api_key {},
              private_key {}
    {
    }

    BtcMarkets(string _api_key,
               string _private_key,
               int32_t _request_timeout = 1000,
               int64_t _timestamp_offset = 0)
        : api_key {_api_key},
          private_key {_private_key},
          request_timeout {_request_timeout},
          timestamp_offset {_timestamp_offset}
    {
        decoded_private_key = base64_decode(private_key);
    }


    json
    order_history(const string& currency,
                  const string& instrument,
                  uint64_t limit,
                  uint64_t since = 0,
                  const string& path =  "/order/history") const
    {
        if (!trading_pair_available(currency, instrument))
            return json {};

        string post_data = "{" + json_pair("currency"   , currency, ",")
                               + json_pair("instrument" , instrument, ",")
                               + json_pair("limit"      , limit, ",")
                               + json_pair("since"      , since)
                               + "}";

        cpr::Response response = post_request(path, post_data);

        if (response.status_code == 200)
            return json::parse(response.text);

        return json {};
    }


    json
    trade_history(const string& currency,
                  const string& instrument,
                  uint64_t limit,
                  uint64_t since = 0) const
    {
        return order_history(currency, instrument,
                             limit, since,
                             "/order/trade/history");
    }


    json
    open_orders(const string& currency,
                  const string& instrument,
                  uint64_t limit,
                  uint64_t since = 0) const
    {
        return order_history(currency, instrument,
                             limit, since,
                             "/order/open");
    }


    /**
     * Basic version of create_order.
     *
     * Volume and price must be uint64_t, as defined in API
     */
    json
    create_order_basic(const string& currency,
                       const string& instrument,
                       uint64_t price,
                       uint64_t volume,
                       const string& order_side,
                       const string& order_type,
                       const string& client_request_id) const
    {
        if (!trading_pair_available(currency, instrument))
            return json {};

        // for some reason, order of json arguments is important! o.O
        // thus we need to manually construct the post_data json string
        // preserving the correct order of json arguments
        // as shown in API example:
        // https://github.com/BTCMarkets/API/wiki/Trading-API#create-an-order

        string post_data = "{" + json_pair("currency"       , currency, ",")
                               + json_pair("instrument"     , instrument, ",")
                               + json_pair("price"          , price, ",")
                               + json_pair("volume"         , volume, ",")
                               + json_pair("orderSide"      , order_side, ",")
                               + json_pair("ordertype"      , order_type, ",")
                               + json_pair("clientRequestId", client_request_id)
                               + "}";

        //cout << post_data << endl;

        cpr::Response response = post_request("/order/create", post_data);

        if (response.status_code == 200)
            return json::parse(response.text);

        return json {};
    }

    /**
     * Normal version of create_order.
     *
     * Volume and price must be doubles, which are
     * converted to uint64_t. This version is more user
     * friendly than the create_order_basic
     */
    json
    create_order(const string& currency,
                 const string& instrument,
                 double price,
                 double volume,
                 const string& order_side,
                 const string& order_type,
                 string client_request_id = "1") const
    {
        return create_order_basic(currency,
                                  instrument,
                                  double_to_uint(price),
                                  double_to_uint(volume),
                                  order_side,
                                  order_type,
                                  client_request_id);
    }


    json
    cancel_order(const vector<uint64_t>& order_id) const
    {
        json j {{"orderIds", order_id}};

        string post_data = j.dump();

        cpr::Response response = post_request("/order/cancel",
                                              post_data);

        if (response.status_code == 200)
            return json::parse(response.text);

        return json {};
    }


    json
    cancel_order(uint64_t order_id) const
    {
        return cancel_order(vector<uint64_t>{order_id});
    }


    json
    order_book(const string& currency,
               const string& instrument,
               const string& path_tmpl = "/market/{inst}/{curr}/orderbook") const
    {
        if (!trading_pair_available(currency, instrument))
            return json {};

        string path {path_tmpl};

        str_replace(path, string("{inst}"), instrument);
        str_replace(path, string("{curr}"), currency);

        cpr::Response response = get_request(path);

        if (response.status_code == 200)
            return json::parse(response.text);

        return json {};
    }


    json
    tick(const string& currency, const string& instrument) const
    {
        if (!trading_pair_available(currency, instrument))
            return json {};

        return order_book(currency, instrument,
                          "/market/{inst}/{curr}/tick");
    }


    json
    trades(const string& currency,
           const string& instrument,
           const string& since_trade_id = "") const
    {
        if (!trading_pair_available(currency, instrument))
            return json {};

        string path = "/market/{inst}/{curr}/trades";

        if (!since_trade_id.empty())
        {
            path += "?since=" + since_trade_id;
        }

        return order_book(currency, instrument, path);
    }


    json
    account_balance() const
    {
        string path = "/account/balance";

        cpr::Response response = get_request(path);

        if (response.status_code == 200)
            return json::parse(response.text);

        return json {};
    }


    json
    order_detail(const vector<uint64_t>& order_id) const
    {
        // only one variable here, so dont need to
        // worry about order
        json j {{"orderIds", order_id}};

        string post_data = j.dump();

        cpr::Response response = post_request("/order/detail",
                                              post_data);

        if (response.status_code == 200)
            return json::parse(response.text);

        return json {};
    }


    json
    order_detail(uint64_t order_id)
    {
        return order_detail(vector<uint64_t>{order_id});
    }


    string
    get_api_key() const
    {
        return api_key;
    }


    string
    get_private_key() const
    {
        return private_key;
    }


    string
    get_decoded_private_key() const
    {
        return decoded_private_key;
    }


    string
    get_decoded_private_key_as_hex() const
    {
        return to_hex(reinterpret_cast<const unsigned char*>(
                              decoded_private_key.c_str()),
                      decoded_private_key.length()) ;
    }


private:

    cpr::Response
    post_request(const string& path, const string& post_data) const
    {
        cpr::Header header = construct_header(path, post_data);

        cpr::Response response = cpr::Post(cpr::Url{base_url + path},
                                           header,
                                           cpr::Body{post_data},
                                           cpr::Timeout{request_timeout});

        if (response.status_code != 200)
        {
            cerr << "status_code: " << response.status_code << endl;
            cerr << "text: " << response.text << endl;
        }

        return response;
    }

    cpr::Response
    get_request(const string& path) const
    {
        cpr::Header header = construct_header(path, "");

        cpr::Response response = cpr::Get(cpr::Url{base_url + path},
                                          header,
                                          cpr::Timeout{request_timeout});

        if (response.status_code != 200)
        {
            cerr << "status_code: " << response.status_code << endl;
            cerr << "text: " << response.text << endl;
        }

        return response;
    }

    bool
    trading_pair_available(const string& currency,
                           const string& instrument) const
    {

        string a_pair {instrument + "/" + currency};

        bool pair_found = (find(begin(trading_pairs),
                               end(trading_pairs), a_pair)
                          != trading_pairs.end());

        if (!pair_found)
        {
            cerr << "Trading pair " << a_pair
                 << " is not available!" << endl;
            cout << "Available instrument/currency trading paris are: " ;

            for(auto p: trading_pairs)
                cout << p << " ";

            cout << endl;
        }

        return pair_found;
    }

    /**
     * Construct header for requests.
     *
     * The header includes current timestamp and
     * signature for authentication purposes.
     */
    cpr::Header
    construct_header(const string& path, const string& post_data) const
    {
        uint64_t timestamp = get_timestamp();

        string signature = request_signature(path, timestamp, post_data);

        cpr::Header header {{"Accept", "application/json"},
                            {"User-Agent", "btc markets C++ client"},
                            {"Content-Type", "application/json"},
                            {"Accept-Charset", "utf-8"},
                            {"apikey", api_key},
                            {"signature", signature},
                            {"timestamp", to_string(timestamp)}};

        return header;
    }

    /**
     * Produce signature for authentication
     */
    string
    request_signature(const string& path,
                      uint64_t timestamp,
                      const string& post_data) const
    {
        string str_to_sign = path + "\n" + to_string(timestamp) + "\n" + post_data;

        std::vector<unsigned char> hmac
                = HMAC_SHA512(get_decoded_private_key(), str_to_sign);

        string encoded_signature = base64_encode(hmac.data(), hmac.size());

        return encoded_signature;
    }

    uint64_t
    get_timestamp() const
    {
        int64_t timestamp_offset_ms {timestamp_offset * 1000};

        return current_timestamp<std::chrono::milliseconds>()
               + timestamp_offset_ms;
    }

};

}

#endif //JSONRPC_TEST_BTCMARKETS_H_H
