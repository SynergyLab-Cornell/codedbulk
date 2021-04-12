/*===================================================================
 * Name     : codec.v
 * Function : network coding codec
 *            oOutput = iCoefficient1 * iInput1 + 
 *                      iCoefficient2 * iInput2
 * by Shih-Hao Tseng
 *                             st688@cornell.edu
=====================================================================*/
/*===================================================================
Prototype:

    codec(
        .iCLK,         (),
        .iCoefficient1 (),
        .iInput1       (),
        .iCoefficient2 (),
        .iInput2       (),
        .oOutput       ()
    );

=====================================================================*/
module codec(
    input          iCLK,     // clock
    input  [ 7: 0] iCoefficient1,
    input  [ 7: 0] iInput1,
    input  [ 7: 0] iCoefficient2,
    input  [ 7: 0] iInput2,
    output [ 7: 0] oOutput

);
    wire   [ 7: 0] wProduct1;
    wire   [ 7: 0] wProduct2;

    assign oOutput = wProduct1 ^ wProduct2;

    multiplicator m1(
        .iCLK          (iCLK),
        .iInput1       (iCoefficient1),
        .iInput2       (iInput1),
        .oOutput       (wProduct1)
    );

    multiplicator m2(
        .iCLK          (iCLK),
        .iInput1       (iCoefficient2),
        .iInput2       (iInput2),
        .oOutput       (wProduct2)
    );

endmodule