#include "oppo/itransport.hpp"
#include <iostream>

#ifdef __APPLE__
#import <Foundation/Foundation.h>
#import <IOBluetooth/IOBluetooth.h>
#include <mutex>
#include <condition_variable>
#include <deque>

@interface OppoMacBluetoothDelegate : NSObject <IOBluetoothRFCOMMChannelDelegate>
@property (nonatomic, assign) BOOL isConnected;
@property (nonatomic, assign) IOBluetoothRFCOMMChannel* channel;
@property (nonatomic, strong) NSMutableArray* rxQueue;
@end

@implementation OppoMacBluetoothDelegate
- (void)rfcommChannelData:(IOBluetoothRFCOMMChannel*)rfcommChannel data:(void*)dataPointer length:(size_t)dataLength {
    NSData* data = [NSData dataWithBytes:dataPointer length:dataLength];
    @synchronized(self.rxQueue) {
        [self.rxQueue addObject:data];
    }
}

- (void)rfcommChannelClosed:(IOBluetoothRFCOMMChannel*)rfcommChannel {
    self.isConnected = NO;
}
@end

namespace oppo {

class MacBluetoothTransport : public ITransport {
private:
    OppoMacBluetoothDelegate* delegate_{nil};
    IOBluetoothRFCOMMChannel* channel_{nil};
    bool connected_{false};

public:
    MacBluetoothTransport() {
        @autoreleasepool {
            delegate_ = [[OppoMacBluetoothDelegate alloc] init];
            delegate_.rxQueue = [[NSMutableArray alloc] init];
        }
    }

    ~MacBluetoothTransport() override {
        disconnect();
    }

    Result<bool> connect(const std::string& mac_address, uint8_t channel_id = 15, int timeout_sec = 5) override {
        (void)timeout_sec;
        disconnect();

        @autoreleasepool {
            std::string clean = normalize_mac(mac_address);
            NSString* macStr = [NSString stdString:clean];
            IOBluetoothDevice* device = [IOBluetoothDevice deviceWithAddressString:macStr];

            if (!device) {
                return FrameError::SocketError;
            }

            IOReturn status = [device openRFCOMMChannelSync:&channel_ withChannelID:channel_id delegate:delegate_];
            if (status != kIOReturnSuccess || !channel_) {
                return FrameError::SocketError;
            }

            delegate_.channel = channel_;
            delegate_.isConnected = YES;
            connected_ = true;
            return true;
        }
    }

    void disconnect() override {
        @autoreleasepool {
            if (channel_) {
                [channel_ closeChannel];
                channel_ = nil;
            }
            if (delegate_) {
                delegate_.isConnected = NO;
            }
            connected_ = false;
        }
    }

    bool is_connected() const override { return connected_; }

    Result<size_t> send(const std::vector<uint8_t>& data) override {
        return send(data.data(), data.size());
    }

    Result<size_t> send(const uint8_t* data, size_t len) override {
        if (!connected_ || !channel_) return FrameError::SocketError;
        @autoreleasepool {
            IOReturn ret = [channel_ writeSync:(void*)data length:len];
            if (ret != kIOReturnSuccess) {
                connected_ = false;
                return FrameError::SocketError;
            }
            return len;
        }
    }

    Result<std::vector<uint8_t>> receive(size_t max_bytes = 1024, int timeout_ms = 2000) override {
        (void)max_bytes;
        (void)timeout_ms;
        if (!connected_ || !delegate_) return FrameError::SocketError;

        @autoreleasepool {
            NSData* packet = nil;
            @synchronized(delegate_.rxQueue) {
                if (delegate_.rxQueue.count > 0) {
                    packet = [delegate_.rxQueue firstObject];
                    [delegate_.rxQueue removeObjectAtIndex:0];
                }
            }

            if (!packet) {
                return FrameError::Timeout;
            }

            const uint8_t* bytes = (const uint8_t*)[packet bytes];
            std::vector<uint8_t> out(bytes, bytes + [packet length]);
            return out;
        }
    }
};

} // namespace oppo
#endif
