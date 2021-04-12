/*===================================================================
 * Name     : multiplicator.v
 * Function : perform finite field multiplication
 *            oOutput = iInput1 * iInput2
 * by Shih-Hao Tseng
 *                             st688@cornell.edu
=====================================================================*/
/*===================================================================
Prototype:

    multiplicator(
        .iCLK          (),
        .iInput1       (),
        .iInput2       (),
        .oOutput       ()
    );

=====================================================================*/
module multiplicator(
    input          iCLK,     // clock
    input  [ 7: 0] iInput1,
    input  [ 7: 0] iInput2,
    output [ 7: 0] oOutput

);
    wire   [ 8: 0] wLog;
    wire   [ 8: 0] wLogMod_ff;
    reg    [ 7: 0] rLog1;
    reg    [ 7: 0] rLog2;
    reg    [ 7: 0] rALog;
    
    assign wLog    = rLog1 + rLog2;
    assign wLogMod_ff = (wLog >= 9'hff) ? wLog - 9'hff : wLog;
    assign oOutput = rALog;

    always @ ( posedge iCLK ) begin
        case( iInput1 )
            8'd000:  rLog1 <= 8'h00;
            8'd001:  rLog1 <= 8'hff;
            8'd002:  rLog1 <= 8'h19;
            8'd003:  rLog1 <= 8'h01;
            8'd004:  rLog1 <= 8'h32;
            8'd005:  rLog1 <= 8'h02;
            8'd006:  rLog1 <= 8'h1a;
            8'd007:  rLog1 <= 8'hc6;
            8'd008:  rLog1 <= 8'h4b;
            8'd009:  rLog1 <= 8'hc7;
            8'd010:  rLog1 <= 8'h1b;
            8'd011:  rLog1 <= 8'h68;
            8'd012:  rLog1 <= 8'h33;
            8'd013:  rLog1 <= 8'hee;
            8'd014:  rLog1 <= 8'hdf;
            8'd015:  rLog1 <= 8'h03;
            8'd016:  rLog1 <= 8'h64;
            8'd017:  rLog1 <= 8'h04;
            8'd018:  rLog1 <= 8'he0;
            8'd019:  rLog1 <= 8'h0e;
            8'd020:  rLog1 <= 8'h34;
            8'd021:  rLog1 <= 8'h8d;
            8'd022:  rLog1 <= 8'h81;
            8'd023:  rLog1 <= 8'hef;
            8'd024:  rLog1 <= 8'h4c;
            8'd025:  rLog1 <= 8'h71;
            8'd026:  rLog1 <= 8'h08;
            8'd027:  rLog1 <= 8'hc8;
            8'd028:  rLog1 <= 8'hf8;
            8'd029:  rLog1 <= 8'h69;
            8'd030:  rLog1 <= 8'h1c;
            8'd031:  rLog1 <= 8'hc1;
            8'd032:  rLog1 <= 8'h7d;
            8'd033:  rLog1 <= 8'hc2;
            8'd034:  rLog1 <= 8'h1d;
            8'd035:  rLog1 <= 8'hb5;
            8'd036:  rLog1 <= 8'hf9;
            8'd037:  rLog1 <= 8'hb9;
            8'd038:  rLog1 <= 8'h27;
            8'd039:  rLog1 <= 8'h6a;
            8'd040:  rLog1 <= 8'h4d;
            8'd041:  rLog1 <= 8'he4;
            8'd042:  rLog1 <= 8'ha6;
            8'd043:  rLog1 <= 8'h72;
            8'd044:  rLog1 <= 8'h9a;
            8'd045:  rLog1 <= 8'hc9;
            8'd046:  rLog1 <= 8'h09;
            8'd047:  rLog1 <= 8'h78;
            8'd048:  rLog1 <= 8'h65;
            8'd049:  rLog1 <= 8'h2f;
            8'd050:  rLog1 <= 8'h8a;
            8'd051:  rLog1 <= 8'h05;
            8'd052:  rLog1 <= 8'h21;
            8'd053:  rLog1 <= 8'h0f;
            8'd054:  rLog1 <= 8'he1;
            8'd055:  rLog1 <= 8'h24;
            8'd056:  rLog1 <= 8'h12;
            8'd057:  rLog1 <= 8'hf0;
            8'd058:  rLog1 <= 8'h82;
            8'd059:  rLog1 <= 8'h45;
            8'd060:  rLog1 <= 8'h35;
            8'd061:  rLog1 <= 8'h93;
            8'd062:  rLog1 <= 8'hda;
            8'd063:  rLog1 <= 8'h8e;
            8'd064:  rLog1 <= 8'h96;
            8'd065:  rLog1 <= 8'h8f;
            8'd066:  rLog1 <= 8'hdb;
            8'd067:  rLog1 <= 8'hbd;
            8'd068:  rLog1 <= 8'h36;
            8'd069:  rLog1 <= 8'hd0;
            8'd070:  rLog1 <= 8'hce;
            8'd071:  rLog1 <= 8'h94;
            8'd072:  rLog1 <= 8'h13;
            8'd073:  rLog1 <= 8'h5c;
            8'd074:  rLog1 <= 8'hd2;
            8'd075:  rLog1 <= 8'hf1;
            8'd076:  rLog1 <= 8'h40;
            8'd077:  rLog1 <= 8'h46;
            8'd078:  rLog1 <= 8'h83;
            8'd079:  rLog1 <= 8'h38;
            8'd080:  rLog1 <= 8'h66;
            8'd081:  rLog1 <= 8'hdd;
            8'd082:  rLog1 <= 8'hfd;
            8'd083:  rLog1 <= 8'h30;
            8'd084:  rLog1 <= 8'hbf;
            8'd085:  rLog1 <= 8'h06;
            8'd086:  rLog1 <= 8'h8b;
            8'd087:  rLog1 <= 8'h62;
            8'd088:  rLog1 <= 8'hb3;
            8'd089:  rLog1 <= 8'h25;
            8'd090:  rLog1 <= 8'he2;
            8'd091:  rLog1 <= 8'h98;
            8'd092:  rLog1 <= 8'h22;
            8'd093:  rLog1 <= 8'h88;
            8'd094:  rLog1 <= 8'h91;
            8'd095:  rLog1 <= 8'h10;
            8'd096:  rLog1 <= 8'h7e;
            8'd097:  rLog1 <= 8'h6e;
            8'd098:  rLog1 <= 8'h48;
            8'd099:  rLog1 <= 8'hc3;
            8'd100:  rLog1 <= 8'ha3;
            8'd101:  rLog1 <= 8'hb6;
            8'd102:  rLog1 <= 8'h1e;
            8'd103:  rLog1 <= 8'h42;
            8'd104:  rLog1 <= 8'h3a;
            8'd105:  rLog1 <= 8'h6b;
            8'd106:  rLog1 <= 8'h28;
            8'd107:  rLog1 <= 8'h54;
            8'd108:  rLog1 <= 8'hfa;
            8'd109:  rLog1 <= 8'h85;
            8'd110:  rLog1 <= 8'h3d;
            8'd111:  rLog1 <= 8'hba;
            8'd112:  rLog1 <= 8'h2b;
            8'd113:  rLog1 <= 8'h79;
            8'd114:  rLog1 <= 8'h0a;
            8'd115:  rLog1 <= 8'h15;
            8'd116:  rLog1 <= 8'h9b;
            8'd117:  rLog1 <= 8'h9f;
            8'd118:  rLog1 <= 8'h5e;
            8'd119:  rLog1 <= 8'hca;
            8'd120:  rLog1 <= 8'h4e;
            8'd121:  rLog1 <= 8'hd4;
            8'd122:  rLog1 <= 8'hac;
            8'd123:  rLog1 <= 8'he5;
            8'd124:  rLog1 <= 8'hf3;
            8'd125:  rLog1 <= 8'h73;
            8'd126:  rLog1 <= 8'ha7;
            8'd127:  rLog1 <= 8'h57;
            8'd128:  rLog1 <= 8'haf;
            8'd129:  rLog1 <= 8'h58;
            8'd130:  rLog1 <= 8'ha8;
            8'd131:  rLog1 <= 8'h50;
            8'd132:  rLog1 <= 8'hf4;
            8'd133:  rLog1 <= 8'hea;
            8'd134:  rLog1 <= 8'hd6;
            8'd135:  rLog1 <= 8'h74;
            8'd136:  rLog1 <= 8'h4f;
            8'd137:  rLog1 <= 8'hae;
            8'd138:  rLog1 <= 8'he9;
            8'd139:  rLog1 <= 8'hd5;
            8'd140:  rLog1 <= 8'he7;
            8'd141:  rLog1 <= 8'he6;
            8'd142:  rLog1 <= 8'had;
            8'd143:  rLog1 <= 8'he8;
            8'd144:  rLog1 <= 8'h2c;
            8'd145:  rLog1 <= 8'hd7;
            8'd146:  rLog1 <= 8'h75;
            8'd147:  rLog1 <= 8'h7a;
            8'd148:  rLog1 <= 8'heb;
            8'd149:  rLog1 <= 8'h16;
            8'd150:  rLog1 <= 8'h0b;
            8'd151:  rLog1 <= 8'hf5;
            8'd152:  rLog1 <= 8'h59;
            8'd153:  rLog1 <= 8'hcb;
            8'd154:  rLog1 <= 8'h5f;
            8'd155:  rLog1 <= 8'hb0;
            8'd156:  rLog1 <= 8'h9c;
            8'd157:  rLog1 <= 8'ha9;
            8'd158:  rLog1 <= 8'h51;
            8'd159:  rLog1 <= 8'ha0;
            8'd160:  rLog1 <= 8'h7f;
            8'd161:  rLog1 <= 8'h0c;
            8'd162:  rLog1 <= 8'hf6;
            8'd163:  rLog1 <= 8'h6f;
            8'd164:  rLog1 <= 8'h17;
            8'd165:  rLog1 <= 8'hc4;
            8'd166:  rLog1 <= 8'h49;
            8'd167:  rLog1 <= 8'hec;
            8'd168:  rLog1 <= 8'hd8;
            8'd169:  rLog1 <= 8'h43;
            8'd170:  rLog1 <= 8'h1f;
            8'd171:  rLog1 <= 8'h2d;
            8'd172:  rLog1 <= 8'ha4;
            8'd173:  rLog1 <= 8'h76;
            8'd174:  rLog1 <= 8'h7b;
            8'd175:  rLog1 <= 8'hb7;
            8'd176:  rLog1 <= 8'hcc;
            8'd177:  rLog1 <= 8'hbb;
            8'd178:  rLog1 <= 8'h3e;
            8'd179:  rLog1 <= 8'h5a;
            8'd180:  rLog1 <= 8'hfb;
            8'd181:  rLog1 <= 8'h60;
            8'd182:  rLog1 <= 8'hb1;
            8'd183:  rLog1 <= 8'h86;
            8'd184:  rLog1 <= 8'h3b;
            8'd185:  rLog1 <= 8'h52;
            8'd186:  rLog1 <= 8'ha1;
            8'd187:  rLog1 <= 8'h6c;
            8'd188:  rLog1 <= 8'haa;
            8'd189:  rLog1 <= 8'h55;
            8'd190:  rLog1 <= 8'h29;
            8'd191:  rLog1 <= 8'h9d;
            8'd192:  rLog1 <= 8'h97;
            8'd193:  rLog1 <= 8'hb2;
            8'd194:  rLog1 <= 8'h87;
            8'd195:  rLog1 <= 8'h90;
            8'd196:  rLog1 <= 8'h61;
            8'd197:  rLog1 <= 8'hbe;
            8'd198:  rLog1 <= 8'hdc;
            8'd199:  rLog1 <= 8'hfc;
            8'd200:  rLog1 <= 8'hbc;
            8'd201:  rLog1 <= 8'h95;
            8'd202:  rLog1 <= 8'hcf;
            8'd203:  rLog1 <= 8'hcd;
            8'd204:  rLog1 <= 8'h37;
            8'd205:  rLog1 <= 8'h3f;
            8'd206:  rLog1 <= 8'h5b;
            8'd207:  rLog1 <= 8'hd1;
            8'd208:  rLog1 <= 8'h53;
            8'd209:  rLog1 <= 8'h39;
            8'd210:  rLog1 <= 8'h84;
            8'd211:  rLog1 <= 8'h3c;
            8'd212:  rLog1 <= 8'h41;
            8'd213:  rLog1 <= 8'ha2;
            8'd214:  rLog1 <= 8'h6d;
            8'd215:  rLog1 <= 8'h47;
            8'd216:  rLog1 <= 8'h14;
            8'd217:  rLog1 <= 8'h2a;
            8'd218:  rLog1 <= 8'h9e;
            8'd219:  rLog1 <= 8'h5d;
            8'd220:  rLog1 <= 8'h56;
            8'd221:  rLog1 <= 8'hf2;
            8'd222:  rLog1 <= 8'hd3;
            8'd223:  rLog1 <= 8'hab;
            8'd224:  rLog1 <= 8'h44;
            8'd225:  rLog1 <= 8'h11;
            8'd226:  rLog1 <= 8'h92;
            8'd227:  rLog1 <= 8'hd9;
            8'd228:  rLog1 <= 8'h23;
            8'd229:  rLog1 <= 8'h20;
            8'd230:  rLog1 <= 8'h2e;
            8'd231:  rLog1 <= 8'h89;
            8'd232:  rLog1 <= 8'hb4;
            8'd233:  rLog1 <= 8'h7c;
            8'd234:  rLog1 <= 8'hb8;
            8'd235:  rLog1 <= 8'h26;
            8'd236:  rLog1 <= 8'h77;
            8'd237:  rLog1 <= 8'h99;
            8'd238:  rLog1 <= 8'he3;
            8'd239:  rLog1 <= 8'ha5;
            8'd240:  rLog1 <= 8'h67;
            8'd241:  rLog1 <= 8'h4a;
            8'd242:  rLog1 <= 8'hed;
            8'd243:  rLog1 <= 8'hde;
            8'd244:  rLog1 <= 8'hc5;
            8'd245:  rLog1 <= 8'h31;
            8'd246:  rLog1 <= 8'hfe;
            8'd247:  rLog1 <= 8'h18;
            8'd248:  rLog1 <= 8'h0d;
            8'd249:  rLog1 <= 8'h63;
            8'd250:  rLog1 <= 8'h8c;
            8'd251:  rLog1 <= 8'h80;
            8'd252:  rLog1 <= 8'hc0;
            8'd253:  rLog1 <= 8'hf7;
            8'd254:  rLog1 <= 8'h70;
            8'd255:  rLog1 <= 8'h07;
        endcase
    end

    always @ ( posedge iCLK ) begin
        case( iInput2 )
            8'd000:  rLog2 <= 8'h00;
            8'd001:  rLog2 <= 8'hff;
            8'd002:  rLog2 <= 8'h19;
            8'd003:  rLog2 <= 8'h01;
            8'd004:  rLog2 <= 8'h32;
            8'd005:  rLog2 <= 8'h02;
            8'd006:  rLog2 <= 8'h1a;
            8'd007:  rLog2 <= 8'hc6;
            8'd008:  rLog2 <= 8'h4b;
            8'd009:  rLog2 <= 8'hc7;
            8'd010:  rLog2 <= 8'h1b;
            8'd011:  rLog2 <= 8'h68;
            8'd012:  rLog2 <= 8'h33;
            8'd013:  rLog2 <= 8'hee;
            8'd014:  rLog2 <= 8'hdf;
            8'd015:  rLog2 <= 8'h03;
            8'd016:  rLog2 <= 8'h64;
            8'd017:  rLog2 <= 8'h04;
            8'd018:  rLog2 <= 8'he0;
            8'd019:  rLog2 <= 8'h0e;
            8'd020:  rLog2 <= 8'h34;
            8'd021:  rLog2 <= 8'h8d;
            8'd022:  rLog2 <= 8'h81;
            8'd023:  rLog2 <= 8'hef;
            8'd024:  rLog2 <= 8'h4c;
            8'd025:  rLog2 <= 8'h71;
            8'd026:  rLog2 <= 8'h08;
            8'd027:  rLog2 <= 8'hc8;
            8'd028:  rLog2 <= 8'hf8;
            8'd029:  rLog2 <= 8'h69;
            8'd030:  rLog2 <= 8'h1c;
            8'd031:  rLog2 <= 8'hc1;
            8'd032:  rLog2 <= 8'h7d;
            8'd033:  rLog2 <= 8'hc2;
            8'd034:  rLog2 <= 8'h1d;
            8'd035:  rLog2 <= 8'hb5;
            8'd036:  rLog2 <= 8'hf9;
            8'd037:  rLog2 <= 8'hb9;
            8'd038:  rLog2 <= 8'h27;
            8'd039:  rLog2 <= 8'h6a;
            8'd040:  rLog2 <= 8'h4d;
            8'd041:  rLog2 <= 8'he4;
            8'd042:  rLog2 <= 8'ha6;
            8'd043:  rLog2 <= 8'h72;
            8'd044:  rLog2 <= 8'h9a;
            8'd045:  rLog2 <= 8'hc9;
            8'd046:  rLog2 <= 8'h09;
            8'd047:  rLog2 <= 8'h78;
            8'd048:  rLog2 <= 8'h65;
            8'd049:  rLog2 <= 8'h2f;
            8'd050:  rLog2 <= 8'h8a;
            8'd051:  rLog2 <= 8'h05;
            8'd052:  rLog2 <= 8'h21;
            8'd053:  rLog2 <= 8'h0f;
            8'd054:  rLog2 <= 8'he1;
            8'd055:  rLog2 <= 8'h24;
            8'd056:  rLog2 <= 8'h12;
            8'd057:  rLog2 <= 8'hf0;
            8'd058:  rLog2 <= 8'h82;
            8'd059:  rLog2 <= 8'h45;
            8'd060:  rLog2 <= 8'h35;
            8'd061:  rLog2 <= 8'h93;
            8'd062:  rLog2 <= 8'hda;
            8'd063:  rLog2 <= 8'h8e;
            8'd064:  rLog2 <= 8'h96;
            8'd065:  rLog2 <= 8'h8f;
            8'd066:  rLog2 <= 8'hdb;
            8'd067:  rLog2 <= 8'hbd;
            8'd068:  rLog2 <= 8'h36;
            8'd069:  rLog2 <= 8'hd0;
            8'd070:  rLog2 <= 8'hce;
            8'd071:  rLog2 <= 8'h94;
            8'd072:  rLog2 <= 8'h13;
            8'd073:  rLog2 <= 8'h5c;
            8'd074:  rLog2 <= 8'hd2;
            8'd075:  rLog2 <= 8'hf1;
            8'd076:  rLog2 <= 8'h40;
            8'd077:  rLog2 <= 8'h46;
            8'd078:  rLog2 <= 8'h83;
            8'd079:  rLog2 <= 8'h38;
            8'd080:  rLog2 <= 8'h66;
            8'd081:  rLog2 <= 8'hdd;
            8'd082:  rLog2 <= 8'hfd;
            8'd083:  rLog2 <= 8'h30;
            8'd084:  rLog2 <= 8'hbf;
            8'd085:  rLog2 <= 8'h06;
            8'd086:  rLog2 <= 8'h8b;
            8'd087:  rLog2 <= 8'h62;
            8'd088:  rLog2 <= 8'hb3;
            8'd089:  rLog2 <= 8'h25;
            8'd090:  rLog2 <= 8'he2;
            8'd091:  rLog2 <= 8'h98;
            8'd092:  rLog2 <= 8'h22;
            8'd093:  rLog2 <= 8'h88;
            8'd094:  rLog2 <= 8'h91;
            8'd095:  rLog2 <= 8'h10;
            8'd096:  rLog2 <= 8'h7e;
            8'd097:  rLog2 <= 8'h6e;
            8'd098:  rLog2 <= 8'h48;
            8'd099:  rLog2 <= 8'hc3;
            8'd100:  rLog2 <= 8'ha3;
            8'd101:  rLog2 <= 8'hb6;
            8'd102:  rLog2 <= 8'h1e;
            8'd103:  rLog2 <= 8'h42;
            8'd104:  rLog2 <= 8'h3a;
            8'd105:  rLog2 <= 8'h6b;
            8'd106:  rLog2 <= 8'h28;
            8'd107:  rLog2 <= 8'h54;
            8'd108:  rLog2 <= 8'hfa;
            8'd109:  rLog2 <= 8'h85;
            8'd110:  rLog2 <= 8'h3d;
            8'd111:  rLog2 <= 8'hba;
            8'd112:  rLog2 <= 8'h2b;
            8'd113:  rLog2 <= 8'h79;
            8'd114:  rLog2 <= 8'h0a;
            8'd115:  rLog2 <= 8'h15;
            8'd116:  rLog2 <= 8'h9b;
            8'd117:  rLog2 <= 8'h9f;
            8'd118:  rLog2 <= 8'h5e;
            8'd119:  rLog2 <= 8'hca;
            8'd120:  rLog2 <= 8'h4e;
            8'd121:  rLog2 <= 8'hd4;
            8'd122:  rLog2 <= 8'hac;
            8'd123:  rLog2 <= 8'he5;
            8'd124:  rLog2 <= 8'hf3;
            8'd125:  rLog2 <= 8'h73;
            8'd126:  rLog2 <= 8'ha7;
            8'd127:  rLog2 <= 8'h57;
            8'd128:  rLog2 <= 8'haf;
            8'd129:  rLog2 <= 8'h58;
            8'd130:  rLog2 <= 8'ha8;
            8'd131:  rLog2 <= 8'h50;
            8'd132:  rLog2 <= 8'hf4;
            8'd133:  rLog2 <= 8'hea;
            8'd134:  rLog2 <= 8'hd6;
            8'd135:  rLog2 <= 8'h74;
            8'd136:  rLog2 <= 8'h4f;
            8'd137:  rLog2 <= 8'hae;
            8'd138:  rLog2 <= 8'he9;
            8'd139:  rLog2 <= 8'hd5;
            8'd140:  rLog2 <= 8'he7;
            8'd141:  rLog2 <= 8'he6;
            8'd142:  rLog2 <= 8'had;
            8'd143:  rLog2 <= 8'he8;
            8'd144:  rLog2 <= 8'h2c;
            8'd145:  rLog2 <= 8'hd7;
            8'd146:  rLog2 <= 8'h75;
            8'd147:  rLog2 <= 8'h7a;
            8'd148:  rLog2 <= 8'heb;
            8'd149:  rLog2 <= 8'h16;
            8'd150:  rLog2 <= 8'h0b;
            8'd151:  rLog2 <= 8'hf5;
            8'd152:  rLog2 <= 8'h59;
            8'd153:  rLog2 <= 8'hcb;
            8'd154:  rLog2 <= 8'h5f;
            8'd155:  rLog2 <= 8'hb0;
            8'd156:  rLog2 <= 8'h9c;
            8'd157:  rLog2 <= 8'ha9;
            8'd158:  rLog2 <= 8'h51;
            8'd159:  rLog2 <= 8'ha0;
            8'd160:  rLog2 <= 8'h7f;
            8'd161:  rLog2 <= 8'h0c;
            8'd162:  rLog2 <= 8'hf6;
            8'd163:  rLog2 <= 8'h6f;
            8'd164:  rLog2 <= 8'h17;
            8'd165:  rLog2 <= 8'hc4;
            8'd166:  rLog2 <= 8'h49;
            8'd167:  rLog2 <= 8'hec;
            8'd168:  rLog2 <= 8'hd8;
            8'd169:  rLog2 <= 8'h43;
            8'd170:  rLog2 <= 8'h1f;
            8'd171:  rLog2 <= 8'h2d;
            8'd172:  rLog2 <= 8'ha4;
            8'd173:  rLog2 <= 8'h76;
            8'd174:  rLog2 <= 8'h7b;
            8'd175:  rLog2 <= 8'hb7;
            8'd176:  rLog2 <= 8'hcc;
            8'd177:  rLog2 <= 8'hbb;
            8'd178:  rLog2 <= 8'h3e;
            8'd179:  rLog2 <= 8'h5a;
            8'd180:  rLog2 <= 8'hfb;
            8'd181:  rLog2 <= 8'h60;
            8'd182:  rLog2 <= 8'hb1;
            8'd183:  rLog2 <= 8'h86;
            8'd184:  rLog2 <= 8'h3b;
            8'd185:  rLog2 <= 8'h52;
            8'd186:  rLog2 <= 8'ha1;
            8'd187:  rLog2 <= 8'h6c;
            8'd188:  rLog2 <= 8'haa;
            8'd189:  rLog2 <= 8'h55;
            8'd190:  rLog2 <= 8'h29;
            8'd191:  rLog2 <= 8'h9d;
            8'd192:  rLog2 <= 8'h97;
            8'd193:  rLog2 <= 8'hb2;
            8'd194:  rLog2 <= 8'h87;
            8'd195:  rLog2 <= 8'h90;
            8'd196:  rLog2 <= 8'h61;
            8'd197:  rLog2 <= 8'hbe;
            8'd198:  rLog2 <= 8'hdc;
            8'd199:  rLog2 <= 8'hfc;
            8'd200:  rLog2 <= 8'hbc;
            8'd201:  rLog2 <= 8'h95;
            8'd202:  rLog2 <= 8'hcf;
            8'd203:  rLog2 <= 8'hcd;
            8'd204:  rLog2 <= 8'h37;
            8'd205:  rLog2 <= 8'h3f;
            8'd206:  rLog2 <= 8'h5b;
            8'd207:  rLog2 <= 8'hd1;
            8'd208:  rLog2 <= 8'h53;
            8'd209:  rLog2 <= 8'h39;
            8'd210:  rLog2 <= 8'h84;
            8'd211:  rLog2 <= 8'h3c;
            8'd212:  rLog2 <= 8'h41;
            8'd213:  rLog2 <= 8'ha2;
            8'd214:  rLog2 <= 8'h6d;
            8'd215:  rLog2 <= 8'h47;
            8'd216:  rLog2 <= 8'h14;
            8'd217:  rLog2 <= 8'h2a;
            8'd218:  rLog2 <= 8'h9e;
            8'd219:  rLog2 <= 8'h5d;
            8'd220:  rLog2 <= 8'h56;
            8'd221:  rLog2 <= 8'hf2;
            8'd222:  rLog2 <= 8'hd3;
            8'd223:  rLog2 <= 8'hab;
            8'd224:  rLog2 <= 8'h44;
            8'd225:  rLog2 <= 8'h11;
            8'd226:  rLog2 <= 8'h92;
            8'd227:  rLog2 <= 8'hd9;
            8'd228:  rLog2 <= 8'h23;
            8'd229:  rLog2 <= 8'h20;
            8'd230:  rLog2 <= 8'h2e;
            8'd231:  rLog2 <= 8'h89;
            8'd232:  rLog2 <= 8'hb4;
            8'd233:  rLog2 <= 8'h7c;
            8'd234:  rLog2 <= 8'hb8;
            8'd235:  rLog2 <= 8'h26;
            8'd236:  rLog2 <= 8'h77;
            8'd237:  rLog2 <= 8'h99;
            8'd238:  rLog2 <= 8'he3;
            8'd239:  rLog2 <= 8'ha5;
            8'd240:  rLog2 <= 8'h67;
            8'd241:  rLog2 <= 8'h4a;
            8'd242:  rLog2 <= 8'hed;
            8'd243:  rLog2 <= 8'hde;
            8'd244:  rLog2 <= 8'hc5;
            8'd245:  rLog2 <= 8'h31;
            8'd246:  rLog2 <= 8'hfe;
            8'd247:  rLog2 <= 8'h18;
            8'd248:  rLog2 <= 8'h0d;
            8'd249:  rLog2 <= 8'h63;
            8'd250:  rLog2 <= 8'h8c;
            8'd251:  rLog2 <= 8'h80;
            8'd252:  rLog2 <= 8'hc0;
            8'd253:  rLog2 <= 8'hf7;
            8'd254:  rLog2 <= 8'h70;
            8'd255:  rLog2 <= 8'h07;
        endcase
    end

    always @ ( posedge iCLK ) begin
        case( wLogMod_ff[7:0] )
            8'd000:  rALog <= 8'h01;
            8'd001:  rALog <= 8'h03;
            8'd002:  rALog <= 8'h05;
            8'd003:  rALog <= 8'h0f;
            8'd004:  rALog <= 8'h11;
            8'd005:  rALog <= 8'h33;
            8'd006:  rALog <= 8'h55;
            8'd007:  rALog <= 8'hff;
            8'd008:  rALog <= 8'h1a;
            8'd009:  rALog <= 8'h2e;
            8'd010:  rALog <= 8'h72;
            8'd011:  rALog <= 8'h96;
            8'd012:  rALog <= 8'ha1;
            8'd013:  rALog <= 8'hf8;
            8'd014:  rALog <= 8'h13;
            8'd015:  rALog <= 8'h35;
            8'd016:  rALog <= 8'h5f;
            8'd017:  rALog <= 8'he1;
            8'd018:  rALog <= 8'h38;
            8'd019:  rALog <= 8'h48;
            8'd020:  rALog <= 8'hd8;
            8'd021:  rALog <= 8'h73;
            8'd022:  rALog <= 8'h95;
            8'd023:  rALog <= 8'ha4;
            8'd024:  rALog <= 8'hf7;
            8'd025:  rALog <= 8'h02;
            8'd026:  rALog <= 8'h06;
            8'd027:  rALog <= 8'h0a;
            8'd028:  rALog <= 8'h1e;
            8'd029:  rALog <= 8'h22;
            8'd030:  rALog <= 8'h66;
            8'd031:  rALog <= 8'haa;
            8'd032:  rALog <= 8'he5;
            8'd033:  rALog <= 8'h34;
            8'd034:  rALog <= 8'h5c;
            8'd035:  rALog <= 8'he4;
            8'd036:  rALog <= 8'h37;
            8'd037:  rALog <= 8'h59;
            8'd038:  rALog <= 8'heb;
            8'd039:  rALog <= 8'h26;
            8'd040:  rALog <= 8'h6a;
            8'd041:  rALog <= 8'hbe;
            8'd042:  rALog <= 8'hd9;
            8'd043:  rALog <= 8'h70;
            8'd044:  rALog <= 8'h90;
            8'd045:  rALog <= 8'hab;
            8'd046:  rALog <= 8'he6;
            8'd047:  rALog <= 8'h31;
            8'd048:  rALog <= 8'h53;
            8'd049:  rALog <= 8'hf5;
            8'd050:  rALog <= 8'h04;
            8'd051:  rALog <= 8'h0c;
            8'd052:  rALog <= 8'h14;
            8'd053:  rALog <= 8'h3c;
            8'd054:  rALog <= 8'h44;
            8'd055:  rALog <= 8'hcc;
            8'd056:  rALog <= 8'h4f;
            8'd057:  rALog <= 8'hd1;
            8'd058:  rALog <= 8'h68;
            8'd059:  rALog <= 8'hb8;
            8'd060:  rALog <= 8'hd3;
            8'd061:  rALog <= 8'h6e;
            8'd062:  rALog <= 8'hb2;
            8'd063:  rALog <= 8'hcd;
            8'd064:  rALog <= 8'h4c;
            8'd065:  rALog <= 8'hd4;
            8'd066:  rALog <= 8'h67;
            8'd067:  rALog <= 8'ha9;
            8'd068:  rALog <= 8'he0;
            8'd069:  rALog <= 8'h3b;
            8'd070:  rALog <= 8'h4d;
            8'd071:  rALog <= 8'hd7;
            8'd072:  rALog <= 8'h62;
            8'd073:  rALog <= 8'ha6;
            8'd074:  rALog <= 8'hf1;
            8'd075:  rALog <= 8'h08;
            8'd076:  rALog <= 8'h18;
            8'd077:  rALog <= 8'h28;
            8'd078:  rALog <= 8'h78;
            8'd079:  rALog <= 8'h88;
            8'd080:  rALog <= 8'h83;
            8'd081:  rALog <= 8'h9e;
            8'd082:  rALog <= 8'hb9;
            8'd083:  rALog <= 8'hd0;
            8'd084:  rALog <= 8'h6b;
            8'd085:  rALog <= 8'hbd;
            8'd086:  rALog <= 8'hdc;
            8'd087:  rALog <= 8'h7f;
            8'd088:  rALog <= 8'h81;
            8'd089:  rALog <= 8'h98;
            8'd090:  rALog <= 8'hb3;
            8'd091:  rALog <= 8'hce;
            8'd092:  rALog <= 8'h49;
            8'd093:  rALog <= 8'hdb;
            8'd094:  rALog <= 8'h76;
            8'd095:  rALog <= 8'h9a;
            8'd096:  rALog <= 8'hb5;
            8'd097:  rALog <= 8'hc4;
            8'd098:  rALog <= 8'h57;
            8'd099:  rALog <= 8'hf9;
            8'd100:  rALog <= 8'h10;
            8'd101:  rALog <= 8'h30;
            8'd102:  rALog <= 8'h50;
            8'd103:  rALog <= 8'hf0;
            8'd104:  rALog <= 8'h0b;
            8'd105:  rALog <= 8'h1d;
            8'd106:  rALog <= 8'h27;
            8'd107:  rALog <= 8'h69;
            8'd108:  rALog <= 8'hbb;
            8'd109:  rALog <= 8'hd6;
            8'd110:  rALog <= 8'h61;
            8'd111:  rALog <= 8'ha3;
            8'd112:  rALog <= 8'hfe;
            8'd113:  rALog <= 8'h19;
            8'd114:  rALog <= 8'h2b;
            8'd115:  rALog <= 8'h7d;
            8'd116:  rALog <= 8'h87;
            8'd117:  rALog <= 8'h92;
            8'd118:  rALog <= 8'had;
            8'd119:  rALog <= 8'hec;
            8'd120:  rALog <= 8'h2f;
            8'd121:  rALog <= 8'h71;
            8'd122:  rALog <= 8'h93;
            8'd123:  rALog <= 8'hae;
            8'd124:  rALog <= 8'he9;
            8'd125:  rALog <= 8'h20;
            8'd126:  rALog <= 8'h60;
            8'd127:  rALog <= 8'ha0;
            8'd128:  rALog <= 8'hfb;
            8'd129:  rALog <= 8'h16;
            8'd130:  rALog <= 8'h3a;
            8'd131:  rALog <= 8'h4e;
            8'd132:  rALog <= 8'hd2;
            8'd133:  rALog <= 8'h6d;
            8'd134:  rALog <= 8'hb7;
            8'd135:  rALog <= 8'hc2;
            8'd136:  rALog <= 8'h5d;
            8'd137:  rALog <= 8'he7;
            8'd138:  rALog <= 8'h32;
            8'd139:  rALog <= 8'h56;
            8'd140:  rALog <= 8'hfa;
            8'd141:  rALog <= 8'h15;
            8'd142:  rALog <= 8'h3f;
            8'd143:  rALog <= 8'h41;
            8'd144:  rALog <= 8'hc3;
            8'd145:  rALog <= 8'h5e;
            8'd146:  rALog <= 8'he2;
            8'd147:  rALog <= 8'h3d;
            8'd148:  rALog <= 8'h47;
            8'd149:  rALog <= 8'hc9;
            8'd150:  rALog <= 8'h40;
            8'd151:  rALog <= 8'hc0;
            8'd152:  rALog <= 8'h5b;
            8'd153:  rALog <= 8'hed;
            8'd154:  rALog <= 8'h2c;
            8'd155:  rALog <= 8'h74;
            8'd156:  rALog <= 8'h9c;
            8'd157:  rALog <= 8'hbf;
            8'd158:  rALog <= 8'hda;
            8'd159:  rALog <= 8'h75;
            8'd160:  rALog <= 8'h9f;
            8'd161:  rALog <= 8'hba;
            8'd162:  rALog <= 8'hd5;
            8'd163:  rALog <= 8'h64;
            8'd164:  rALog <= 8'hac;
            8'd165:  rALog <= 8'hef;
            8'd166:  rALog <= 8'h2a;
            8'd167:  rALog <= 8'h7e;
            8'd168:  rALog <= 8'h82;
            8'd169:  rALog <= 8'h9d;
            8'd170:  rALog <= 8'hbc;
            8'd171:  rALog <= 8'hdf;
            8'd172:  rALog <= 8'h7a;
            8'd173:  rALog <= 8'h8e;
            8'd174:  rALog <= 8'h89;
            8'd175:  rALog <= 8'h80;
            8'd176:  rALog <= 8'h9b;
            8'd177:  rALog <= 8'hb6;
            8'd178:  rALog <= 8'hc1;
            8'd179:  rALog <= 8'h58;
            8'd180:  rALog <= 8'he8;
            8'd181:  rALog <= 8'h23;
            8'd182:  rALog <= 8'h65;
            8'd183:  rALog <= 8'haf;
            8'd184:  rALog <= 8'hea;
            8'd185:  rALog <= 8'h25;
            8'd186:  rALog <= 8'h6f;
            8'd187:  rALog <= 8'hb1;
            8'd188:  rALog <= 8'hc8;
            8'd189:  rALog <= 8'h43;
            8'd190:  rALog <= 8'hc5;
            8'd191:  rALog <= 8'h54;
            8'd192:  rALog <= 8'hfc;
            8'd193:  rALog <= 8'h1f;
            8'd194:  rALog <= 8'h21;
            8'd195:  rALog <= 8'h63;
            8'd196:  rALog <= 8'ha5;
            8'd197:  rALog <= 8'hf4;
            8'd198:  rALog <= 8'h07;
            8'd199:  rALog <= 8'h09;
            8'd200:  rALog <= 8'h1b;
            8'd201:  rALog <= 8'h2d;
            8'd202:  rALog <= 8'h77;
            8'd203:  rALog <= 8'h99;
            8'd204:  rALog <= 8'hb0;
            8'd205:  rALog <= 8'hcb;
            8'd206:  rALog <= 8'h46;
            8'd207:  rALog <= 8'hca;
            8'd208:  rALog <= 8'h45;
            8'd209:  rALog <= 8'hcf;
            8'd210:  rALog <= 8'h4a;
            8'd211:  rALog <= 8'hde;
            8'd212:  rALog <= 8'h79;
            8'd213:  rALog <= 8'h8b;
            8'd214:  rALog <= 8'h86;
            8'd215:  rALog <= 8'h91;
            8'd216:  rALog <= 8'ha8;
            8'd217:  rALog <= 8'he3;
            8'd218:  rALog <= 8'h3e;
            8'd219:  rALog <= 8'h42;
            8'd220:  rALog <= 8'hc6;
            8'd221:  rALog <= 8'h51;
            8'd222:  rALog <= 8'hf3;
            8'd223:  rALog <= 8'h0e;
            8'd224:  rALog <= 8'h12;
            8'd225:  rALog <= 8'h36;
            8'd226:  rALog <= 8'h5a;
            8'd227:  rALog <= 8'hee;
            8'd228:  rALog <= 8'h29;
            8'd229:  rALog <= 8'h7b;
            8'd230:  rALog <= 8'h8d;
            8'd231:  rALog <= 8'h8c;
            8'd232:  rALog <= 8'h8f;
            8'd233:  rALog <= 8'h8a;
            8'd234:  rALog <= 8'h85;
            8'd235:  rALog <= 8'h94;
            8'd236:  rALog <= 8'ha7;
            8'd237:  rALog <= 8'hf2;
            8'd238:  rALog <= 8'h0d;
            8'd239:  rALog <= 8'h17;
            8'd240:  rALog <= 8'h39;
            8'd241:  rALog <= 8'h4b;
            8'd242:  rALog <= 8'hdd;
            8'd243:  rALog <= 8'h7c;
            8'd244:  rALog <= 8'h84;
            8'd245:  rALog <= 8'h97;
            8'd246:  rALog <= 8'ha2;
            8'd247:  rALog <= 8'hfd;
            8'd248:  rALog <= 8'h1c;
            8'd249:  rALog <= 8'h24;
            8'd250:  rALog <= 8'h6c;
            8'd251:  rALog <= 8'hb4;
            8'd252:  rALog <= 8'hc7;
            8'd253:  rALog <= 8'h52;
            8'd254:  rALog <= 8'hf6;
            8'd255:  rALog <= 8'h01;
        endcase
    end

endmodule