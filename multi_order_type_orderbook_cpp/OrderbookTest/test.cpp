/**
 * @file OrderbookTests.cpp
 * @brief Parameterized unit tests for Orderbook, reading action scripts from files.
 * 
 * This test suite verifies order book behavior by processing sequences of
 * Add/Modify/Cancel commands from text files and checking the final state
 * (total order count, bid count, ask count) against expected results.
 */

#include "pch.h"

#include "../Orderbook.cpp"

namespace googletest = ::testing;

/**
 * @brief Types of actions that can be performed on the order book.
 */
enum class ActionType
{
    Add,
    Cancel,
    Modify,
};

/**
 * @brief Represents one action parsed from a test file line.
 */
struct Information
{
    ActionType type_;
    OrderType orderType_;
    Side side_;
    Price price_;
    Quantity quantity_;
    OrderId orderId_;
};

using Informations = std::vector<Information>;

/**
 * @brief Expected final state of the order book after all actions.
 */
struct Result
{
    std::size_t allCount_;
    std::size_t bidCount_;
    std::size_t askCount_;
};

using Results = std::vector<Result>;

/**
 * @brief Parses test input files into a sequence of actions and an expected result.
 */
struct InputHandler
{
private:
    /**
     * @brief Converts a string view to a numeric value.
     * @throws std::logic_error if value is negative.
     */
    std::uint32_t ToNumber(const std::string_view& str) const
    {
        std::int64_t value{};
        std::from_chars(str.data(), str.data() + str.size(), value);
        if (value < 0)
            throw std::logic_error("Value is below zero.");
        return static_cast<std::uint32_t>(value);
    }

    /**
     * @brief Attempts to parse a result line (starting with 'R').
     * @param str The line to parse.
     * @param result Output parameter to store parsed result.
     * @return True if parsing succeeded.
     */
    bool TryParseResult(const std::string_view& str, Result& result) const
    {
        if (str.at(0) != 'R')
            return false;

        auto values = Split(str, ' ');
        result.allCount_ = ToNumber(values[1]);
        result.bidCount_ = ToNumber(values[2]);
        result.askCount_ = ToNumber(values[3]);

        return true;
    }

    /**
     * @brief Attempts to parse an action line (starting with 'A', 'M', or 'C').
     * @param str The line to parse.
     * @param action Output parameter to store parsed action.
     * @return True if parsing succeeded.
     */
    bool TryParseInformation(const std::string_view& str, Information& action) const
    {
        auto value = str.at(0);
        auto values = Split(str, ' ');
        if (value == 'A')
        {
            action.type_ = ActionType::Add;
            action.side_ = ParseSide(values[1]);
            action.orderType_ = ParseOrderType(values[2]);
            action.price_ = ParsePrice(values[3]);
            action.quantity_ = ParseQuantity(values[4]);
            action.orderId_ = ParseOrderId(values[5]);
        }
        else if (value == 'M')
        {
            action.type_ = ActionType::Modify;
            action.orderId_ = ParseOrderId(values[1]);
            action.side_ = ParseSide(values[2]);
            action.price_ = ParsePrice(values[3]);
            action.quantity_ = ParseQuantity(values[4]);
        }
        else if (value == 'C')
        {
            action.type_ = ActionType::Cancel;
            action.orderId_ = ParseOrderId(values[1]);
        }
        else return false;

        return true;
    }

    /**
     * @brief Splits a string view by a delimiter.
     * @return Vector of string views pointing to parts of the original string.
     */
    std::vector<std::string_view> Split(const std::string_view& str, char delimeter) const
    {
        std::vector<std::string_view> columns;
        columns.reserve(5);
        std::size_t start_index{}, end_index{};
        while ((end_index = str.find(delimeter, start_index)) && end_index != std::string::npos)
        {
            auto distance = end_index - start_index;
            auto column = str.substr(start_index, distance);
            start_index = end_index + 1;
            columns.push_back(column);
        }
        columns.push_back(str.substr(start_index));
        return columns;
    }

    /**
     * @brief Converts a string to Side enum.
     */
    Side ParseSide(const std::string_view& str) const
    {
        if (str == "B")
            return Side::Buy;
        else if (str == "S")
            return Side::Sell;
        else throw std::logic_error("Unknown Side");
    }

    /**
     * @brief Converts a string to OrderType enum.
     */
    OrderType ParseOrderType(const std::string_view& str) const
    {
        if (str == "FillAndKill")
            return OrderType::FillAndKill;
        else if (str == "GoodTillCancel")
            return OrderType::GoodTillCancel;
        else if (str == "GoodForDay")
            return OrderType::GoodForDay;
        else if (str == "FillOrKill")
            return OrderType::FillOrKill;
        else if (str == "Market")
            return OrderType::Market;
        else throw std::logic_error("Unknown OrderType");
    }

    Price ParsePrice(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Unknown Price");

        return ToNumber(str);
    }

    Quantity ParseQuantity(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Unknown Quantity");

        return ToNumber(str);
    }

    OrderId ParseOrderId(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Empty OrderId");

        return static_cast<OrderId>(ToNumber(str));
    }

public:
    /**
     * @brief Reads a test file and returns all actions and the expected result.
     * @param path Path to the test file.
     * @return Tuple of actions and expected result.
     * @throws std::logic_error if file format is invalid.
     */
    std::tuple<Informations, Result> GetInformations(const std::filesystem::path& path) const
    {
        Informations actions;
        actions.reserve(1'000);

        std::string line;
        std::ifstream file{ path };
        while (std::getline(file, line))
        {
            if (line.empty())
                break;

            const bool isResult = line.at(0) == 'R';
            const bool isAction = !isResult;
            
            if (isAction)
            {
                Information action;

                auto isValid = TryParseInformation(line, action);
                if (!isValid)
                    continue;

                actions.push_back(action);
            }
            else
            {
                if (!file.eof())
                    throw std::logic_error("Result should only be specified at the end.");

                Result result;

                auto isValid = TryParseResult(line, result);
                if (!isValid)
                    continue;

                return { actions, result };
            }

        }

        throw std::logic_error("No result specified.");
    }
};

/**
 * @brief Test fixture for parameterized order book tests.
 * 
 * Sets up the path to the test files directory.
 */
class OrderbookTestsFixture : public googletest::TestWithParam<const char*> 
{
private:
    const static inline std::filesystem::path Root{ std::filesystem::current_path() };
    const static inline std::filesystem::path TestFolder{ "TestFiles" };
public:
    const static inline std::filesystem::path TestFolderPath{ Root / TestFolder };
};

/**
 * @brief Parameterized test case that runs one test file.
 * 
 * For each test file, it:
 * 1. Parses actions and expected result.
 * 2. Executes actions on an Orderbook.
 * 3. Asserts final state matches expected counts.
 */
TEST_P(OrderbookTestsFixture, OrderbookTestSuite)
{
    // Arrange
    const auto file = OrderbookTestsFixture::TestFolderPath / GetParam();

    InputHandler handler;
    const auto [actions, result] = handler.GetInformations(file);

    auto GetOrder = [](const Information& action)
    {
        return std::make_shared<Order>(
            action.orderType_,
            action.orderId_,
            action.side_,
            action.price_,
            action.quantity_);
    };

    auto GetOrderModify = [](const Information& action)
    {
        return OrderModify
        {
            action.orderId_,
            action.side_,
            action.price_,
            action.quantity_,
        };
    };

    // Act
    Orderbook orderbook;
    for (const auto& action : actions)
    {
        switch (action.type_)
        {
        case ActionType::Add:
        {
            const Trades& trades = orderbook.AddOrder(GetOrder(action));
        }
        break;
        case ActionType::Modify:
        {
            const Trades& trades = orderbook.ModifyOrder(GetOrderModify(action));
        }
        break;
        case ActionType::Cancel:
        {
            orderbook.CancelOrder(action.orderId_);
        }
        break;
        default:
            throw std::logic_error("Unsupported Action.");
        }
    }

    // Assert
    const auto& orderbookInfos = orderbook.GetOrderInfos();
    ASSERT_EQ(orderbook.Size(), result.allCount_);
    ASSERT_EQ(orderbookInfos.GetBids().size(), result.bidCount_);
    ASSERT_EQ(orderbookInfos.GetAsks().size(), result.askCount_);
}

/**
 * @brief List of test input files to run.
 * 
 * Each file contains a sequence of actions and a final 'R' line with expected counts.
 */
INSTANTIATE_TEST_CASE_P(Tests, OrderbookTestsFixture, googletest::ValuesIn({
    "Match_GoodTillCancel.txt",
    "Match_FillAndKill.txt",
    "Match_FillOrKill_Hit.txt",
    "Match_FillOrKill_Miss.txt",
    "Cancel_Success.txt",
    "Modify_Side.txt",
    "Match_Market.txt"
}));