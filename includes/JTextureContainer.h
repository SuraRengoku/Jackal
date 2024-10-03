//
// Created by jonas on 2024/6/25.
//

#ifndef JTEXTUREContainer_H
#define JTEXTUREContainer_H

#include <memory>

namespace JackalRenderer{
    using uchar = unsigned char;
    using uint = unsigned int;
    class JTextureContainer {
    public:
        using ptr = std::shared_ptr<JTextureContainer>;
        JTextureContainer(std::uint16_t width, std::uint16_t height);
        virtual ~JTextureContainer();

        std::uint16_t getWidth() const { return width; }
        std::uint16_t getHeight() const { return height; }
        virtual std::uint32_t read(const std::uint16_t &x, const std::uint16_t &y) const; //coordinator
        virtual void read(const std::uint16_t &x, const std::uint16_t &y, uchar &r, uchar &g, uchar &b, uchar &a) const;

    protected:
        std::uint16_t width, height;
        std::uint32_t *tData;
        void loadTexture(const unsigned int &nElement, uchar *data, const std::uint16_t &width, const std::uint16_t &height, const int &channel);
        virtual uint xyToIndex(const std::uint16_t &x, const std::uint16_t &y) const = 0;
        // when a pure virtual function is called in children class, the corresponding overrided function is called
        void freeTexture();
    };

    class JLinearTextureContainer final : public JTextureContainer {
    public:
        using ptr = std::shared_ptr<JLinearTextureContainer>;
        JLinearTextureContainer(uchar *data, std::uint16_t width, std::uint16_t height, int channel);
        virtual ~JLinearTextureContainer() = default;
    private:
        virtual uint xyToIndex(const std::uint16_t &x, const std::uint16_t &y) const override;
    };

    class JTilingTextureContainer final : public JTextureContainer {
    public:
        using ptr = std::shared_ptr<JTilingTextureContainer>;
        JTilingTextureContainer(uchar *data, std::uint16_t width, std::uint16_t height, int channel);
        virtual ~JTilingTextureContainer() = default;
    private:
        static constexpr int tileBase4 = 4;
        static constexpr int tileBase16 = 16;
        int widthInTiles = 0;
        int heightInTiles = 0;
        virtual uint xyToIndex(const std::uint16_t &x, const std::uint16_t &y) const override;
    };

    class JSwizzlingTextureContainer final : public JTextureContainer {
    public:
        using ptr = std::shared_ptr<JSwizzlingTextureContainer>;
        JSwizzlingTextureContainer(uchar *data, std::uint16_t width, std::uint16_t height, int channel);
        virtual ~JSwizzlingTextureContainer() = default;
    private:
        static constexpr int tileBase32 = 32;
        static constexpr int tileBase1024 = 1024;
        static constexpr int bits32 = 5;
        static constexpr int bits1024 = 10;

        int widthInTiles = 0;
        int heightInTiles = 0;
        virtual uint xyToIndex(const std::uint16_t &x, const std::uint16_t &y) const override;

        static void decodeMortonCurve(std::uint8_t &x, std::uint8_t &y, const std::uint16_t &ind) {
            x = 0, y = 0;
            for(int i = 0; i < bits32; ++i) {
                x |= (ind & (1 << (2 * i))) >> i;
                y |= (ind & (1 << (2 * i + 1))) >> (i + 1);
            }
        }

        inline static void encodeMortonCurve(const std::uint8_t &x, const std::uint8_t &y, std::uint16_t &ind) {
            ind = 0;
            ind |= ((x & (1 << 0)) << (0)) | ((y & (1 << 0)) << (1));
            ind |= ((x & (1 << 1)) << (1)) | ((y & (1 << 1)) << (2));
            ind |= ((x & (1 << 2)) << (2)) | ((y & (1 << 2)) << (3));
            ind |= ((x & (1 << 3)) << (3)) | ((y & (1 << 3)) << (4));
            ind |= ((x & (1 << 4)) << (4)) | ((y & (1 << 4)) << (5));
            ind |= ((x & (1 << 5)) << (5)) | ((y & (1 << 5)) << (6));
            ind |= ((x & (1 << 6)) << (6)) | ((y & (1 << 6)) << (7));
            ind |= ((x & (1 << 7)) << (7)) | ((y & (1 << 7)) << (8));
            ind |= ((x & (1 << 8)) << (8)) | ((y & (1 << 8)) << (9));
            ind |= ((x & (1 << 9)) << (9)) | ((y & (1 << 9)) << (10));
        }
    };
}

#endif //JTEXTUREContainer_H
