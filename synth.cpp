#include "synth.hpp"

namespace synth {

  uint32_t prng_xorshift_state = 0x32B71700;

  uint32_t prng_xorshift_next() {
    uint32_t x = prng_xorshift_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    prng_xorshift_state = x;
    return x;
  }

  int32_t prng_normal() {
    // rough approximation of a normal distribution
    uint32_t r0 = prng_xorshift_next();
    uint32_t r1 = prng_xorshift_next();
    uint32_t n = ((r0 & 0xffff) + (r1 & 0xffff) + (r0 >> 16) + (r1 >> 16)) / 2;
    return n - 0xffff;
  }

  uint16_t volume = 0xffff;
  const int16_t sine_waveform [256] = {-32768,-32758,-32729,-32679,-32610,-32522,-32413,-32286,-32138,-31972,-31786,-31581,-31357,-31114,-30853,-30572,-30274,-29957,-29622,-29269,-28899,-28511,-28106,-27684,-27246,-26791,-26320,-25833,-25330,-24812,-24279,-23732,-23170,-22595,-22006,-21403,-20788,-20160,-19520,-18868,-18205,-17531,-16846,-16151,-15447,-14733,-14010,-13279,-12540,-11793,-11039,-10279,-9512,-8740,-7962,-7180,-6393,-5602,-4808,-4011,-3212,-2411,-1608,-804,0,804,1608,2411,3212,4011,4808,5602,6393,7180,7962,8740,9512,10279,11039,11793,12540,13279,14010,14733,15447,16151,16846,17531,18205,18868,19520,20160,20788,21403,22006,22595,23170,23732,24279,24812,25330,25833,26320,26791,27246,27684,28106,28511,28899,29269,29622,29957,30274,30572,30853,31114,31357,31581,31786,31972,32138,32286,32413,32522,32610,32679,32729,32758,32767,32758,32729,32679,32610,32522,32413,32286,32138,31972,31786,31581,31357,31114,30853,30572,30274,29957,29622,29269,28899,28511,28106,27684,27246,26791,26320,25833,25330,24812,24279,23732,23170,22595,22006,21403,20788,20160,19520,18868,18205,17531,16846,16151,15447,14733,14010,13279,12540,11793,11039,10279,9512,8740,7962,7180,6393,5602,4808,4011,3212,2411,1608,804,0,-804,-1608,-2411,-3212,-4011,-4808,-5602,-6393,-7180,-7962,-8740,-9512,-10279,-11039,-11793,-12540,-13279,-14010,-14733,-15447,-16151,-16846,-17531,-18205,-18868,-19520,-20160,-20788,-21403,-22006,-22595,-23170,-23732,-24279,-24812,-25330,-25833,-26320,-26791,-27246,-27684,-28106,-28511,-28899,-29269,-29622,-29957,-30274,-30572,-30853,-31114,-31357,-31581,-31786,-31972,-32138,-32286,-32413,-32522,-32610,-32679,-32729,-32758};
  const int16_t violin_waveform [256] = {-3066,-1662,-345,917,2135,3301,4409,5504,6500,7490,8429,9308,10123,10899,11600,12250,12843,13370,13808,14187,14480,14674,14777,14774,14678,14511,14257,13905,13403,12702,11749,10440,8818,7175,5863,4896,4236,3760,3439,3245,3236,3459,3927,4684,5738,7125,8854,10954,13412,16149,19094,22071,24930,27487,29595,31181,32229,32741,32767,32291,31329,29904,28072,25916,23469,20861,18166,15449,12774,10268,7938,5821,3948,2362,1048,13,-738,-1208,-1405,-1403,-1234,-866,-273,587,1739,3221,4992,7006,9050,10995,12642,14020,15064,15803,16301,16582,16672,16574,16275,15766,15051,14100,12918,11527,9886,7989,5900,3604,1117,-1505,-4237,-7050,-9874,-12676,-15425,-18054,-20502,-22750,-24813,-26645,-28239,-29572,-30683,-31541,-32168,-32579,-32767,-32766,-32661,-32449,-32097,-31619,-30926,-30063,-28917,-27574,-25971,-24297,-22733,-21381,-20353,-19634,-19174,-18938,-18877,-18973,-19192,-19525,-19938,-20454,-20995,-21615,-22253,-22953,-23645,-24350,-25056,-25784,-26462,-27111,-27746,-28304,-28823,-29282,-29632,-29898,-29987,-29788,-29310,-28579,-27623,-26471,-25201,-23856,-22471,-21077,-19665,-18319,-17025,-15795,-14604,-13493,-12467,-11496,-10583,-9759,-8968,-8264,-7595,-7010,-6460,-5967,-5530,-5127,-4778,-4478,-4211,-3991,-3803,-3650,-3537,-3461,-3416,-3399,-3409,-3456,-3575,-3807,-4089,-4202,-4175,-3954,-3644,-3379,-3183,-3059,-2970,-2941,-3094,-3525,-3994,-4335,-4543,-4651,-4693,-4688,-4669,-4632,-4577,-4496,-4384,-4222,-4018,-3822,-3707,-3663,-3659,-3682,-3750,-3875,-4077,-4344,-4728,-5152,-5589,-5941,-6126,-6184,-6111,-5942,-5693,-5361,-4984,-4586,-4113,-3620};
  const int16_t reed_waveform [256] = {-1080,-695,-340,0,328,651,957,1264,1555,1840,2117,2385,2640,2899,3144,3374,3606,3838,4070,4285,4483,4697,4894,5089,5267,5458,5632,5804,5974,6126,6291,6438,6583,6725,6865,7001,7134,7251,7366,7489,7597,7702,7803,7901,7985,8077,8164,8241,8333,8432,8549,8673,8807,8948,9098,9269,9461,9663,9875,10097,10359,10616,10917,11212,11553,11907,12311,12728,13178,13662,14203,14780,15393,16089,16847,17689,18616,19651,20814,22144,23648,25314,27104,28826,30311,31395,32128,32570,32767,32507,31644,30065,27644,24352,20191,15378,10327,5369,866,-3081,-6429,-9206,-11482,-13309,-14740,-15828,-16618,-17146,-17438,-17519,-17409,-17171,-16900,-16603,-16293,-15972,-15648,-15312,-14968,-14640,-14298,-13966,-13635,-13306,-12968,-12647,-12333,-12026,-11712,-11408,-11097,-10797,-10507,-10213,-9932,-9646,-9374,-9098,-8836,-8554,-8304,-8034,-7780,-7523,-7263,-7020,-6775,-6528,-6298,-6048,-5815,-5580,-5364,-5126,-4907,-4686,-4465,-4242,-4017,-3813,-3607,-3379,-3171,-2962,-2774,-2563,-2373,-2160,-1968,-1776,-1582,-1344,-1033,-722,-386,-50,284,644,1028,1435,1865,2291,2762,3252,3758,4301,4878,5503,6147,6801,7491,8161,8796,9365,9831,10178,10427,10582,10661,10593,9991,9221,8243,6993,5352,3092,-289,-6332,-17203,-23626,-26908,-28997,-30417,-31454,-32211,-32767,-32728,-32295,-31768,-31149,-30422,-29593,-28660,-27636,-26544,-25387,-24191,-22986,-21801,-20625,-19487,-18372,-17308,-16277,-15283,-14352,-13439,-12570,-11722,-10946,-10169,-9440,-8737,-8034,-7381,-6755,-6154,-5556,-5008,-4439,-3921,-3405,-2915,-2429,-1969,-1511};
  const int16_t pluckedguitar_waveform [256] = {4379,5456,6533,7556,8613,9687,10684,11639,12542,13355,14079,14714,15240,15684,16021,16288,16470,16594,16656,16667,16610,16480,16257,15938,15516,14963,14262,13402,12338,11058,9517,7760,5770,3681,1561,-389,-2078,-3384,-4387,-5089,-5556,-5796,-5864,-5781,-5428,-4656,-3425,-1708,434,2968,5718,8561,11420,14094,16623,18928,20991,22847,24492,25965,27234,28340,29291,30095,30786,31365,31821,32181,32449,32632,32738,32767,32725,32623,32475,32220,31853,31368,30741,29983,29065,27978,26715,25273,23614,21777,19687,17398,14935,12336,9610,6911,4318,1987,-10,-1564,-2632,-3223,-3400,-3366,-3056,-2243,-327,3758,7687,9999,11480,12523,13284,13833,14249,14534,14752,14884,14954,14971,14916,14743,14431,13943,13274,12329,11127,9623,7814,5797,3789,1847,142,-1348,-2656,-3744,-4693,-5477,-6139,-6713,-7181,-7574,-7896,-8154,-8354,-8500,-8606,-8667,-8690,-8675,-8613,-8488,-8296,-8029,-7643,-7157,-6488,-5600,-4431,-2797,-524,2698,6213,8951,10735,11935,12763,13349,13724,13969,14104,14148,14094,13872,13452,12821,11968,10833,9385,7665,5568,3156,441,-2556,-5709,-8879,-12010,-14965,-17710,-20219,-22400,-24334,-26024,-27477,-28704,-29746,-30590,-31291,-31824,-32233,-32523,-32696,-32767,-32732,-32565,-32250,-31768,-31106,-30227,-29110,-27705,-26021,-23978,-21557,-18757,-15548,-12061,-8411,-4736,-1348,1646,4181,6255,7862,9091,9957,10532,10840,10926,10843,10585,10072,9120,7342,3980,-29,-2585,-4118,-5153,-5838,-6311,-6644,-6858,-6967,-7009,-6972,-6847,-6642,-6341,-5966,-5493,-4956,-4290,-3569,-2766,-1851,-838,282,1496,2850};
  const int16_t piano_waveform [256] = {-4623,-2997,-1405,152,1687,3182,4646,6114,7543,8938,10313,11656,12986,14270,15553,16780,17997,19174,20332,21442,22526,23578,24574,25552,26468,27355,28191,28970,29688,30355,30950,31488,31930,32295,32557,32727,32767,32616,32193,31501,30676,29903,29266,28769,28359,28049,27788,27597,27447,27333,27252,27201,27174,27170,27172,27177,27184,27192,27200,27210,27220,27230,27240,27251,27262,27272,27283,27293,27303,27313,27323,27332,27340,27349,27356,27364,27370,27376,27382,27386,27391,27394,27396,27398,27399,27399,27398,27396,27393,27389,27384,27378,27370,27362,27352,27340,27328,27314,27298,27281,27262,27240,27218,27194,27166,27138,27106,27072,27036,26997,26956,26909,26860,26807,26752,26693,26628,26558,26481,26399,26308,26212,26105,25987,25862,25718,25559,25375,25169,24846,24314,23520,22392,20832,18734,16009,12623,8795,4881,1233,-2011,-4878,-7368,-9526,-11446,-13154,-14657,-16025,-17241,-18333,-19325,-20221,-21043,-21793,-22459,-23075,-23630,-24127,-24581,-24993,-25356,-25692,-25991,-26248,-26481,-26682,-26854,-26999,-27122,-27220,-27297,-27352,-27385,-27400,-27401,-27388,-27364,-27327,-27278,-27219,-27149,-27064,-26974,-26869,-26755,-26632,-26501,-26362,-26209,-26057,-25891,-25728,-25552,-25372,-25199,-25013,-24826,-24638,-24449,-24261,-24083,-23898,-23715,-23536,-23370,-23199,-23043,-22884,-22739,-22602,-22472,-22350,-22237,-22187,-22241,-22409,-22710,-23134,-23706,-24395,-25205,-26079,-27015,-27919,-28767,-29542,-30254,-30855,-31363,-31781,-32124,-32374,-32568,-32690,-32755,-32767,-32627,-32209,-31583,-30781,-29824,-28759,-27593,-26349,-25023,-23605,-22135,-20602,-19021,-17384,-15711,-13977,-12193,-10368,-8510,-6603};


  bool is_audio_playing() {
    if(volume == 0) {
      return false;
    }

    bool any_channel_playing = false;
    for(int c = 0; c < CHANNEL_COUNT; c++) {
      if(channels[c].volume > 0 && channels[c].adsr_phase != ADSRPhase::OFF) {
        any_channel_playing = true;
      }
    }

    return any_channel_playing;
  }

  int16_t get_audio_frame() {
    int32_t sample = 0;  // used to combine channel output

    for(int c = 0; c < CHANNEL_COUNT; c++) {

      auto &channel = channels[c];

      // increment the waveform position counter. this provides an
      // Q16 fixed point value representing how far through
      // the current waveform we are
      channel.waveform_offset += ((channel.frequency * 256) << 8) / sample_rate;

      if(channel.adsr_phase == ADSRPhase::OFF) {
        continue;
      }

//      if ((channel.adsr_frame >= channel.adsr_end_frame) && (channel.adsr_phase != ADSRPhase::SUSTAIN)) {
      if (channel.adsr_frame >= channel.adsr_end_frame) {
        switch (channel.adsr_phase) {
          case ADSRPhase::ATTACK:
            channel.trigger_decay();
            break;
          case ADSRPhase::DECAY:
            channel.trigger_sustain();
            break;
          case ADSRPhase::SUSTAIN:
            channel.trigger_release();
            break;
          case ADSRPhase::RELEASE:
            channel.off();
            break;
          default:
            break;
        }
      }

      channel.adsr += channel.adsr_step;
      channel.adsr_frame++;

      if(channel.waveform_offset & 0x10000) {
        // if the waveform offset overflows then generate a new
        // random noise sample
        channel.noise = prng_normal();
      }

      channel.waveform_offset &= 0xffff;

      // check if any waveforms are active for this channel
      if(channel.waveforms) {
        uint8_t waveform_count = 0;
        int32_t channel_sample = 0;

        // check if channel frequency is 0; if so, then sample shall be 0
        if (channel.frequency == 0) channel_sample +=0;
        // if channel frequency is not 0, then process sample
        else {

          if(channel.waveforms & Waveform::NOISE) {
            channel_sample += channel.noise;
            waveform_count++;
          }

          if(channel.waveforms & Waveform::SAW) {
            channel_sample += (int32_t)channel.waveform_offset - 0x7fff;
            waveform_count++;
          }

          // creates a triangle wave of ^
          if (channel.waveforms & Waveform::TRIANGLE) {
            if (channel.waveform_offset < 0x7fff) { // initial quarter up slope
              channel_sample += int32_t(channel.waveform_offset * 2) - int32_t(0x7fff);
            }
            else { // final quarter up slope
              channel_sample += int32_t(0x7fff) - ((int32_t(channel.waveform_offset) - int32_t(0x7fff)) * 2);
            }
            waveform_count++;
          }

          if (channel.waveforms & Waveform::SQUARE) {
            channel_sample += (channel.waveform_offset < channel.pulse_width) ? 0x7fff : -0x7fff;
            waveform_count++;
          }

          if(channel.waveforms & Waveform::SINE) {
            // the sine_waveform sample contains 256 samples in
            // total so we'll just use the most significant bits
            // of the current waveform position to index into it
            channel_sample += sine_waveform[channel.waveform_offset >> 8];
            waveform_count++;
          }

          if(channel.waveforms & Waveform::PIANO) {
            // the waveform sample contains 256 samples in
            // total so we'll just use the most significant bits
            // of the current waveform position to index into it
            channel_sample += piano_waveform[channel.waveform_offset >> 8];
            waveform_count++;
          }

          if(channel.waveforms & Waveform::REED) {
            // the waveform sample contains 256 samples in
            // total so we'll just use the most significant bits
            // of the current waveform position to index into it
            channel_sample += reed_waveform[channel.waveform_offset >> 8];
            waveform_count++;
          }

          if(channel.waveforms & Waveform::PLUCKEDGUITAR) {
            // the waveform sample contains 256 samples in
            // total so we'll just use the most significant bits
            // of the current waveform position to index into it
            channel_sample += pluckedguitar_waveform[channel.waveform_offset >> 8];
            waveform_count++;
          }

          if(channel.waveforms & Waveform::VIOLIN) {
            // the waveform sample contains 256 samples in
            // total so we'll just use the most significant bits
            // of the current waveform position to index into it
            channel_sample += violin_waveform[channel.waveform_offset >> 8];
            waveform_count++;
          }

          if(channel.waveforms & Waveform::WAVE) {

            // fix to allow buffer loading at the first call
            if (channel.wave_buf_pos == 0) {
              if(channel.wave_buffer_callback)
                  channel.wave_buffer_callback(channel);
            }
            channel_sample += channel.wave_buffer[channel.wave_buf_pos];
            if (++channel.wave_buf_pos == 64) {
              channel.wave_buf_pos = 0;
            }

//          channel_sample += channel.wave_buffer[channel.wave_buf_pos];
//          if (++channel.wave_buf_pos == 64) {
//            channel.wave_buf_pos = 0;
//            if(channel.wave_buffer_callback)
//                channel.wave_buffer_callback(channel);
//          }
            waveform_count++;
          }
        }

        channel_sample = channel_sample / waveform_count;

        channel_sample = (int64_t(channel_sample) * int32_t(channel.adsr >> 8)) >> 16;

        // apply channel volume
        channel_sample = (int64_t(channel_sample) * int32_t(channel.volume)) >> 16;

        // apply channel filter
        //if (channel.filter_enable) {
          //float filter_epow = 1 - expf(-(1.0f / 22050.0f) * 2.0f * pi * int32_t(channel.filter_cutoff_frequency));
          //channel_sample += (channel_sample - channel.filter_last_sample) * filter_epow;
        //}

        //channel.filter_last_sample = channel_sample;

        // combine channel sample into the final sample
        sample += channel_sample;
      }
    }

    sample = (int64_t(sample) * int32_t(volume)) >> 16;

    // clip result to 16-bit
    sample = sample <= -0x8000 ? -0x8000 : (sample > 0x7fff ? 0x7fff : sample);
    return sample;
  }
}