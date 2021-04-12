/*===================================================================
 * Name     : codec_128bit.v
 * Function : a wrapper of 16 network coding codecs
 *            oOutput = iCoefficient1 * iInput1 + 
 *                      iCoefficient2 * iInput2
 * by Shih-Hao Tseng
 *                             st688@cornell.edu
=====================================================================*/
/*===================================================================
Prototype:

    codec_128bit(
        .iCLK          (),
        .iCoefficient1 (),
        .iInput1       (),  // 64 bits
        .iCoefficient2 (),
        .iInput2       (),  // 64 bits
        .oOutput       ()
    );

=====================================================================*/
module codec_128bit(
    input           iCLK,           // clock
    input  [  7: 0] iCoefficient1,
    input  [127: 0] iInput1,        // 64 bits
    input  [  7: 0] iCoefficient2,
    input  [127: 0] iInput2,        // 64 bits
    output [127: 0] oOutput
);

    genvar bit_location;
    for (bit_location = 0; bit_location < 128; bit_location = bit_location + 8) begin:wrap
        codec codec_ins(
            .iCLK          (iCLK),
            .iCoefficient1 (iCoefficient1),
            .iInput1       (iInput1[bit_location+7:bit_location]),
            .iCoefficient2 (iCoefficient2),
            .iInput2       (iInput2[bit_location+7:bit_location]),
            .oOutput       (oOutput[bit_location+7:bit_location])
        );
    end

endmodule
