% Input parameters

% cfir00
fs = 40e6;            % sampling freq
fc = 1.8e6;           % cut off freq
L  = 160;             % filter order

fs = 4e6;             % sampling freq
fc = 0.095e6;           % cut off freq
L  = 260;             % filter order

% Calculate the filter
f0 = fc/(fs/2);       % norm. frequency
b = fir1(L, f0);      % FIR filter coefficients

% Get the frequency response
h  = freqz(b);       
nf = ( 0:pi/(length(h)-1):pi )';
f  = nf*fs/(2*pi);

% Plot the filter response
plot( f, 20*log10( abs(h) ) );
xlabel( "Hz" );
ylabel( "dB" );
title( "FIR filter response" );
legend( sprintf("order=%d", length(b)-1) );
axis( [0,fs/2,-90,1] );
grid on;

% saveas seems to be coming up unresolved, but is in documenation
%saveas( 1, "plot1.png", "png" ); % Save figure 1 as png
print -dpng fig1.png;


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
% Create the Xilinx .coe file
%

B  = 16;                      %% 16 bit coeffs
b  = b/max(b);                %% Floating point coefficients
bz = round(b*power(2,B-1)-1); %% Fixed point coefficients

fid = fopen( "cfir.coe", "w");
fprintf(fid,";\n");
fprintf(fid,"; CIC Fir compensation filter\n");
fprintf(fid,"; Coefficients generated by Octave\n");
fprintf(fid,"; Fs (sampling freq)      =%d [Hz]\n",fs);
fprintf(fid,"; Fc (cutoff freq)        =%d [Hz]\n",fc);
fprintf(fid,"; F0 (normalized cutoff)  =%f\n",f0);
fprintf(fid,"; L  (number coefs)       =%d\n",L);
fprintf(fid,";\n");
fprintf(fid,"radix=10;\n");
fprintf(fid,"coefdata=\n");
for i=1:L 
   fprintf(fid,"%d,\n",bz(i));
endfor
fprintf(fid,";\n");
fclose(fid);

