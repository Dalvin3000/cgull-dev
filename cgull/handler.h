#pragma once


namespace CGull
{

    class Handler
    {
    public:
        virtual ~Handler() = 0;

        virtual void setFinisher() = 0;
    };
};
