/*
 * This file is part of the Peripheral Template Library project.
 *
 * Copyright (c) 2012 Paul Sokolovsky <pfalcon@users.sourceforge.net>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ADC_MSP430_HPP
#define _ADC_MSP430_HPP

namespace PTL {

struct ADCDefaultConfig
{
    enum Reference {
        VCC_VSS   = SREF_0,
        VREFp_VSS = SREF_1,
        // TODO: Add more
    };

    enum SampleAndHold {
        SAMPLE_4CLK  = ADC10SHT_0,
        SAMPLE_8CLK  = ADC10SHT_1,
        SAMPLE_16CLK = ADC10SHT_2,
        SAMPLE_64CLK = ADC10SHT_3
    };

    enum SampleRate {
        // Reference buffer supports up to ~200 ksps
        SAMPLE_RATE_HIGH = 0,
        // Reference buffer supports up to ~50 ksps
        SAMPLE_RATE_LOW  = ADC10SR,
    };

    enum ReferenceOutput {
        REF_OUT_OFF = 0,
        REF_OUT_ON  = REFOUT,
    };

    enum ReferenceBuffer {
        REF_BUFFER_ALWAYS_ON = 0,
        REF_BUFFER_BURST = REFBURST,
    };

    enum SampleRepeat {
        SINGLE = 0,
        MULTI = MSC,
    };

    enum VREFVoltage {
        VREF1_5V = 0,
        VREF2_5V = REF2_5V,
    };

    enum VREFState {
        VREF_OFF = 0,
        VREF_ON = REFON,
    };


    // ADC10CTL1
    enum SampleSignal {
        SOFTWARE = SHS_0,
        TIMER0_A_OUT1 = SHS_1,
        TIMER0_A_OUT0 = SHS_2,
        TIMER0_A_OUT2 = SHS_3,
    };

    enum ValueFormat {
        RIGHT_ADJUST = 0,
        LEFT_ADJUST = ADC10DF,
    };

    enum SampleSignalPolarity {
        POLARITY_NORMAL = 0,
        POLARITY_INVERTED = ISSH,
    };

    // TODO: redo clocks using standard API
    enum Clock {
        ADC10OSC = ADC10SSEL_0,
        ACLK = ADC10SSEL_1,
        MCLK = ADC10SSEL_2,
        SMCLK = ADC10SSEL_3
    };

    enum ConversionMode {
        SINGLE_CHANNEL = CONSEQ_0,
        CHANNEL_SEQUENCE = CONSEQ_1,
        REPEAT_CHANNEL = CONSEQ_2,
        REPEAT_SEQUENCE = CONSEQ_3,
    };


    const static int mode = SINGLE_CHANNEL;
    const static int repeat = SINGLE;

    const static int reference = VREFp_VSS;
    const static int ref_out = REF_OUT_OFF;
    const static int ref_buffer = REF_BUFFER_BURST;
    const static int vref_voltage = VREF2_5V;
    const static int vref_state = VREF_ON;

    const static int sample_time = SAMPLE_16CLK;
    const static int sample_rate = SAMPLE_RATE_LOW;
    const static int sample_signal = SOFTWARE;
    const static int sample_signal_polarity = POLARITY_NORMAL;

    const static int value_format = RIGHT_ADJUST;

    // TODO: redo clocks using standard API
    const static int clock = SMCLK;
};


class ADC
{
public:
    typedef uint16_t width;
    const static int MAX_VALUE = (1 << 10) - 1;


    enum Channel {
        A0 = 0 << 12, A1 = 1 << 12, A2 = 2 << 12, A3 = 3 << 12,
        A4 = 4 << 12, A5 = 5 << 12, A6 = 6 << 12, A7 = 7 << 12,
        VeREFp = 8 << 12, VREFm = 9 << 12, VeREFm = VREFm, TEMP = 10 << 12, VCC_HALF = 11 << 12,
        A12 = 12 << 12, A13 = 13 << 12, A14 = 14 << 12, A15 = 15 << 12
    };

    /*
     * Enable channel for ADC usage.
     */
    static void enable_channel(unsigned channel)
    {
#ifdef ADC10AE1
        if (channel >= A12) {
            ADC10AE1 |= 1 << ((channel >> 12) - 8);
            return;
        }
#endif
        if (channel <= A7) {
            ADC10AE0 |= 1 << (channel >> 12);
        }
    }

    static void set_channel(enum Channel channel)
    {
        ADC10CTL1 = (ADC10CTL1 & ~INCH_15) | channel;
    }

    // Allow to change config
    static void unlock() { ADC10CTL0 &= ~ENC; }
    // Lock config to perform conversion
    static void lock() { ADC10CTL0 |= ENC; }
    // Start conversion (automatically locks config)
    static void start() { ADC10CTL0 |= ADC10ON; ADC10CTL0 |= ENC | ADC10SC; }
    // Busy-wait for conversion end
    static void wait() { while (!(ADC10CTL0 & ADC10IFG)); ADC10CTL0 &= ~ADC10IFG; }
    // Get conversion result
    static width value() { return ADC10MEM; }
    static void disable()
    {
        // Unlock is really required here
        unlock();
        ADC10CTL0 &= ~(REFOUT | REFON | ADC10ON | ENC);
    }

    NOINLINE static width easy_sample(enum Channel channel)
    {
        enable_channel(channel);
        unlock();
        set_channel(channel);
        // Let Vref to settle
        __delay_cycles(30);

        start();
        wait();

        int val = value();
    // Disable all features of ADC
//    unlock();
//    disable();
        return val;
    }


    template <class c>
    static uint16_t ctl0()
    {
        return c::reference | c::sample_time| c::sample_rate | c::ref_out
               | c::ref_buffer | c::repeat | c::vref_voltage | c::vref_state;
    }
    template <class c>
    static uint16_t ctl1()
    {
        return c::sample_signal | c::value_format | c::sample_signal_polarity | c::mode | c::clock;
    }
    template <class config_>
    static void config()
    {
        unlock();
        ADC10CTL0 = (ADC10CTL0 & 0xf) | ctl0<config_>();
        ADC10CTL1 = (ADC10CTL1 & (INCH_15 | ADC10DIV_7)) | ctl1<config_>();
    }
};

} // namespace

#endif //_ADC_MSP430_HPP
