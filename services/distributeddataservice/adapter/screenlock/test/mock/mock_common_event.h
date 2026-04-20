/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MOCK_COMMON_EVENT_H
#define MOCK_COMMON_EVENT_H

#include <string>
#include <functional>
#include <memory>
#include <map>
#include <vector>

namespace OHOS {
namespace EventFwk {

// Forward declarations for common event classes
class CommonEventData;
class CommonEventSubscribeInfo;
class CommonEventSubscriber;

// Mock CommonEventSupport class - must be defined before CommonEventManager
class CommonEventSupport {
public:
    static constexpr const char* COMMON_EVENT_SCREEN_UNLOCKED = "usual.event.SCREEN_UNLOCKED";
    static constexpr const char* COMMON_EVENT_SCREEN_LOCKED = "usual.event.SCREEN_LOCKED";
};

// Mock Want class
class Want {
public:
    Want() = default;
    ~Want() = default;

    void SetAction(const std::string &action)
    {
        action_ = action;
    }

    std::string GetAction() const
    {
        return action_;
    }

    void SetParam(const std::string &key, int32_t value)
    {
        intParams_[key] = value;
    }

    int32_t GetIntParam(const std::string &key, int32_t defaultVal) const
    {
        auto it = intParams_.find(key);
        if (it != intParams_.end()) {
            return it->second;
        }
        return defaultVal;
    }

private:
    std::string action_;
    std::map<std::string, int32_t> intParams_;
};

// Mock CommonEventData class
class CommonEventData {
public:
    explicit CommonEventData(const Want &want) : want_(want) {}
    ~CommonEventData() = default;

    Want GetWant() const
    {
        return want_;
    }

private:
    Want want_;
};

// Mock MatchingSkills class
class MatchingSkills {
public:
    MatchingSkills() = default;
    ~MatchingSkills() = default;

    void AddEvent(const std::string &event)
    {
        events_.push_back(event);
    }

    const std::vector<std::string>& GetEvents() const
    {
        return events_;
    }

private:
    std::vector<std::string> events_;
};

// Mock CommonEventSubscribeInfo class
class CommonEventSubscribeInfo {
public:
    explicit CommonEventSubscribeInfo(const MatchingSkills &skills)
        : skills_(skills) {}
    ~CommonEventSubscribeInfo() = default;

    const MatchingSkills& GetMatchingSkills() const
    {
        return skills_;
    }

private:
    MatchingSkills skills_;
};

// Mock CommonEventSubscriber base class
class CommonEventSubscriber {
public:
    explicit CommonEventSubscriber(const CommonEventSubscribeInfo &info)
        : subscribeInfo_(info) {}
    virtual ~CommonEventSubscriber() = default;

    virtual void OnReceiveEvent(const CommonEventData &event) = 0;

protected:
    CommonEventSubscribeInfo subscribeInfo_;
};

// Mock CommonEventManager class
class CommonEventManager {
public:
    static bool SubscribeCommonEvent(const std::shared_ptr<CommonEventSubscriber> &subscriber)
    {
        subscribers_.push_back(subscriber);
        return subscribeResult_;
    }

    static bool UnSubscribeCommonEvent(const std::shared_ptr<CommonEventSubscriber> &subscriber)
    {
        auto it = std::find_if(subscribers_.begin(), subscribers_.end(),
            [&subscriber](const std::weak_ptr<CommonEventSubscriber> &weak) {
                auto locked = weak.lock();
                return locked && locked.get() == subscriber.get();
            });
        if (it != subscribers_.end()) {
            subscribers_.erase(it);
        }
        return unsubscribeResult_;
    }

    // Mock control methods
    static void SetSubscribeResult(bool result)
    {
        subscribeResult_ = result;
    }

    static void SetUnsubscribeResult(bool result)
    {
        unsubscribeResult_ = result;
    }

    static void Reset()
    {
        subscribeResult_ = true;
        unsubscribeResult_ = true;
        subscribers_.clear();
    }

    // Publish event to all subscribers (for testing)
    static void PublishEvent(const CommonEventData &event)
    {
        // Clean up expired weak pointers
        subscribers_.erase(
            std::remove_if(subscribers_.begin(), subscribers_.end(),
                [](const std::weak_ptr<CommonEventSubscriber> &weak) {
                    return weak.expired();
                }),
            subscribers_.end());

        // Notify all subscribers
        for (const auto &weakSubscriber : subscribers_) {
            auto subscriber = weakSubscriber.lock();
            if (subscriber) {
                subscriber->OnReceiveEvent(event);
            }
        }
    }

    // Helper method to publish screen unlock event
    static void PublishScreenUnlockEvent(int32_t userId)
    {
        Want want;
        want.SetAction(CommonEventSupport::COMMON_EVENT_SCREEN_UNLOCKED);
        want.SetParam("userId", userId);
        PublishEvent(CommonEventData(want));
    }

    // Helper method to publish screen locked event
    static void PublishScreenLockedEvent(int32_t userId)
    {
        Want want;
        want.SetAction(CommonEventSupport::COMMON_EVENT_SCREEN_LOCKED);
        want.SetParam("userId", userId);
        PublishEvent(CommonEventData(want));
    }

private:
    static bool subscribeResult_;
    static bool unsubscribeResult_;
    static std::vector<std::weak_ptr<CommonEventSubscriber>> subscribers_;
};

} // namespace EventFwk
} // namespace OHOS

#endif // MOCK_COMMON_EVENT_H
