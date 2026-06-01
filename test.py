from diff_testing.cudaq import cudaqTesting

source_code = r"""
#include <cudaq.h>
#include <iostream>
#include <cmath> 
#include <vector> 
#include <complex> 

using namespace std::complex_literals;
struct sub_0 { 
void operator()(cudaq::qubit& sing_34,cudaq::qubit& sing_41,cudaq::qubit& sing_48,cudaq::qubit& sing_55, double sing_63,double sing_70) __qpu__ {
    x<cudaq::ctrl>(sing_41, sing_55, sing_48);
    h<cudaq::ctrl>(sing_48, sing_34);
    h<cudaq::ctrl>(sing_48, sing_55);
    if (mz(sing_55)){
        swap(sing_34, sing_41);
        x(sing_55);
        if (mz(sing_48)){
            t(sing_41);
            h<cudaq::ctrl>(sing_41, sing_55);
            h(sing_41);
            s(sing_48);
            x<cudaq::ctrl>(sing_55, sing_41);
            r1((M_PI/2.0), sing_41);
}
        s<cudaq::adj>(sing_34);
        rx(sing_63, sing_48);
        h(sing_48);
        y(sing_41);
        x(sing_41);
        ry(sing_63, sing_48);
}   else {
        s<cudaq::adj>(sing_48);
        swap(sing_34, sing_48);
        ry(7.852677, sing_41);
        rz(sing_63, sing_55);
        z(sing_34);
        r1((M_PI/4.0), sing_41);
}
    x<cudaq::ctrl>(sing_41, sing_48);
    t<cudaq::adj>(sing_48);
    rz<cudaq::ctrl>((M_PI/4.0), sing_34, sing_48);
    ry((M_PI/2.0), sing_34);


}};
struct sub_1 { 
void operator()(cudaq::qubit& sing_527,cudaq::qubit& sing_534,cudaq::qubit& sing_541,cudaq::qubit& sing_548, double sing_556,double sing_563) __qpu__ {
    if (!mz(sing_541)){
        rz(3.122816, sing_541);
        if (mz(sing_527)){
            sub_0{}(sing_534, sing_541, sing_548, sing_527, (M_PI/4.0), sing_556);
            x<cudaq::ctrl>(sing_534, sing_527, sing_541);
            y(sing_527);
            ry(1.269552, sing_534);
            sub_0{}(sing_541, sing_527, sing_534, sing_548, sing_556, M_PI);
            sub_0{}(sing_527, sing_541, sing_548, sing_534, 7.573367, 7.264977);
            sub_0{}(sing_527, sing_548, sing_541, sing_534, sing_556, 9.589556);
            sub_0{}(sing_534, sing_548, sing_541, sing_527, (M_PI/4.0), 4.382733);
}       else {
            sub_0{}(sing_527, sing_548, sing_534, sing_541, sing_563, (M_PI/4.0));
            sub_0{}(sing_541, sing_548, sing_534, sing_527, sing_563, sing_556);
            sub_0{}(sing_541, sing_534, sing_527, sing_548, sing_556, (M_PI/2.0));
            s(sing_548);
            x<cudaq::ctrl>(sing_548, sing_527, sing_541);
            z<cudaq::ctrl>(sing_527, sing_541);
            sub_0{}(sing_548, sing_534, sing_541, sing_527, (M_PI/4.0), sing_563);
            sub_0{}(sing_541, sing_534, sing_548, sing_527, (M_PI/4.0), M_PI);
}
        rx(M_PI, sing_548);
        sub_0{}(sing_534, sing_527, sing_541, sing_548, 7.922984, M_PI);
        sub_0{}(sing_534, sing_541, sing_527, sing_548, M_PI, (M_PI/4.0));
        rx<cudaq::ctrl>(M_PI, sing_534, sing_527);
        sub_0{}(sing_534, sing_548, sing_541, sing_527, sing_563, sing_556);
        ry<cudaq::ctrl>(7.245564, sing_541, sing_527);
        h<cudaq::ctrl>(sing_541, sing_548);
        rz<cudaq::ctrl>(9.727753, sing_541, sing_548);
}
    sub_0{}(sing_541, sing_548, sing_527, sing_534, (M_PI/4.0), 3.599778);
    x<cudaq::ctrl>(sing_541, sing_534, sing_527);
    h<cudaq::ctrl>(sing_534, sing_527);
    s(sing_548);
    sub_0{}(sing_548, sing_534, sing_541, sing_527, 7.655629, (M_PI/2.0));
    sub_0{}(sing_548, sing_527, sing_534, sing_541, (M_PI/4.0), sing_563);


}};
struct sub_2 { 
void operator()(cudaq::qubit& sing_1313,cudaq::qubit& sing_1320,cudaq::qubit& sing_1327,cudaq::qubit& sing_1334, double sing_1342,double sing_1349) __qpu__ {
    if (mz(sing_1320)){
        if (mz(sing_1334)){
            sub_0{}(sing_1327, sing_1320, sing_1313, sing_1334, (M_PI/4.0), sing_1342);
            sub_1{}(sing_1320, sing_1327, sing_1313, sing_1334, (M_PI/2.0), (M_PI/2.0));
            t(sing_1320);
            r1(1.299198, sing_1320);
            sub_1{}(sing_1313, sing_1327, sing_1334, sing_1320, sing_1349, M_PI);
            sub_1{}(sing_1320, sing_1327, sing_1334, sing_1313, 4.270589, 6.547113);
            sub_0{}(sing_1313, sing_1327, sing_1320, sing_1334, 9.524955, sing_1349);
            swap(sing_1313, sing_1327);
            r1(3.372535, sing_1320);
}
        z(sing_1327);
        sub_1{}(sing_1313, sing_1320, sing_1334, sing_1327, M_PI, sing_1349);
        sub_1{}(sing_1313, sing_1327, sing_1320, sing_1334, M_PI, sing_1349);
        t(sing_1313);
        sub_0{}(sing_1320, sing_1334, sing_1327, sing_1313, sing_1349, (M_PI/4.0));
        rx(sing_1349, sing_1334);
        sub_1{}(sing_1313, sing_1334, sing_1320, sing_1327, M_PI, 8.205976);
        sub_0{}(sing_1313, sing_1334, sing_1327, sing_1320, (M_PI/4.0), 0.050618);
        sub_0{}(sing_1313, sing_1334, sing_1320, sing_1327, 9.770300, (M_PI/4.0));
}   else {
        t<cudaq::adj>(sing_1327);
        sub_1{}(sing_1334, sing_1320, sing_1327, sing_1313, (M_PI/2.0), (M_PI/2.0));
        sub_1{}(sing_1334, sing_1320, sing_1313, sing_1327, sing_1349, M_PI);
        sub_1{}(sing_1313, sing_1327, sing_1334, sing_1320, 3.238322, 5.522258);
        sub_0{}(sing_1313, sing_1327, sing_1320, sing_1334, 4.508995, (M_PI/4.0));
        ry((M_PI/2.0), sing_1313);
        r1((M_PI/2.0), sing_1334);
        swap(sing_1320, sing_1334);
        sub_1{}(sing_1313, sing_1327, sing_1334, sing_1320, sing_1349, sing_1349);
}
    h(sing_1327);
    sub_0{}(sing_1313, sing_1320, sing_1327, sing_1334, (M_PI/4.0), 0.039363);
    sub_0{}(sing_1334, sing_1320, sing_1313, sing_1327, (M_PI/4.0), 2.311603);
    sub_0{}(sing_1320, sing_1334, sing_1313, sing_1327, sing_1342, 5.133362);
    x<cudaq::ctrl>(sing_1327, sing_1334);
    sub_1{}(sing_1320, sing_1313, sing_1327, sing_1334, sing_1342, (M_PI/2.0));
    t<cudaq::adj>(sing_1334);
    sub_0{}(sing_1320, sing_1313, sing_1334, sing_1327, (M_PI/2.0), 2.709168);


}};
struct sub_3 { 
void operator()(cudaq::qubit& sing_2209,cudaq::qubit& sing_2216,cudaq::qubit& sing_2223,cudaq::qubit& sing_2230, double sing_2238,double sing_2245) __qpu__ {
    if (!mz(sing_2223)){
        if (mz(sing_2216)){
            u3(sing_2245, M_PI, sing_2238, sing_2216);
            s<cudaq::adj>(sing_2209);
            rx<cudaq::ctrl>(9.815293, sing_2209, sing_2223);
            sub_0{}(sing_2216, sing_2209, sing_2223, sing_2230, sing_2245, (M_PI/4.0));
            sub_2{}(sing_2223, sing_2216, sing_2230, sing_2209, (M_PI/2.0), sing_2238);
            z(sing_2223);
            r1((M_PI/2.0), sing_2223);
            rx<cudaq::ctrl>(5.492203, sing_2216, sing_2209);
            z<cudaq::ctrl>(sing_2223, sing_2230);
}
        sub_1{}(sing_2209, sing_2216, sing_2223, sing_2230, sing_2238, 0.816023);
        sub_2{}(sing_2209, sing_2230, sing_2223, sing_2216, (M_PI/2.0), (M_PI/2.0));
        sub_0{}(sing_2223, sing_2209, sing_2230, sing_2216, sing_2238, sing_2245);
        sub_2{}(sing_2216, sing_2230, sing_2209, sing_2223, sing_2245, (M_PI/4.0));
        sub_1{}(sing_2223, sing_2216, sing_2209, sing_2230, 4.665749, 1.576718);
        ry<cudaq::ctrl>(sing_2238, sing_2230, sing_2216);
        ry<cudaq::ctrl>(sing_2245, sing_2230, sing_2209);
}
    sub_2{}(sing_2230, sing_2209, sing_2216, sing_2223, sing_2245, (M_PI/2.0));
    sub_0{}(sing_2216, sing_2209, sing_2223, sing_2230, 8.250043, (M_PI/2.0));
    sub_0{}(sing_2223, sing_2230, sing_2209, sing_2216, M_PI, (M_PI/4.0));
    ry<cudaq::ctrl>(5.805630, sing_2209, sing_2223);
    x<cudaq::ctrl>(sing_2230, sing_2216, sing_2209);
    sub_2{}(sing_2230, sing_2209, sing_2216, sing_2223, M_PI, 6.589416);
    r1(8.397644, sing_2230);
    sub_2{}(sing_2230, sing_2209, sing_2216, sing_2223, M_PI, 7.724624);
    sub_2{}(sing_2223, sing_2230, sing_2216, sing_2209, (M_PI/4.0), (M_PI/2.0));


}};
struct sub_4 { 
void operator()(cudaq::qubit& sing_2880,cudaq::qubit& sing_2887,cudaq::qubit& sing_2894,cudaq::qubit& sing_2901, double sing_2909,double sing_2916) __qpu__ {
    sub_2{}(sing_2880, sing_2901, sing_2894, sing_2887, sing_2909, sing_2916);
    if (!mz(sing_2894)){
        if (!mz(sing_2880)){
            h(sing_2894);
            t(sing_2887);
            sub_1{}(sing_2887, sing_2894, sing_2880, sing_2901, (M_PI/4.0), sing_2909);
            ry<cudaq::ctrl>(7.379446, sing_2901, sing_2887);
            z(sing_2894);
            sub_3{}(sing_2880, sing_2894, sing_2887, sing_2901, 5.788974, (M_PI/4.0));
            swap(sing_2901, sing_2880);
            sub_1{}(sing_2901, sing_2880, sing_2894, sing_2887, sing_2916, (M_PI/4.0));
            x(sing_2901);
}       else {
            sub_1{}(sing_2880, sing_2901, sing_2894, sing_2887, sing_2909, (M_PI/2.0));
            rz(0.616081, sing_2887);
            s(sing_2894);
            x<cudaq::ctrl>(sing_2880, sing_2894, sing_2887);
            sub_3{}(sing_2901, sing_2894, sing_2887, sing_2880, 8.511998, sing_2916);
            sub_0{}(sing_2901, sing_2887, sing_2894, sing_2880, sing_2916, 8.948496);
            h<cudaq::ctrl>(sing_2901, sing_2887);
            sub_3{}(sing_2901, sing_2894, sing_2880, sing_2887, sing_2909, 6.912494);
            sub_1{}(sing_2880, sing_2894, sing_2901, sing_2887, 8.299641, M_PI);
}
        sub_0{}(sing_2894, sing_2901, sing_2887, sing_2880, (M_PI/2.0), (M_PI/4.0));
        rz(sing_2916, sing_2880);
        sub_0{}(sing_2887, sing_2901, sing_2880, sing_2894, sing_2916, sing_2909);
        sub_0{}(sing_2880, sing_2901, sing_2887, sing_2894, 5.710631, (M_PI/2.0));
        sub_3{}(sing_2887, sing_2901, sing_2894, sing_2880, (M_PI/4.0), sing_2916);
        rx<cudaq::ctrl>(6.004715, sing_2887, sing_2880);
        s(sing_2894);
}
    s(sing_2887);
    sub_1{}(sing_2880, sing_2901, sing_2887, sing_2894, 9.988250, (M_PI/4.0));
    sub_3{}(sing_2880, sing_2887, sing_2901, sing_2894, 1.953568, 6.473647);
    z(sing_2880);
    ry<cudaq::ctrl>((M_PI/2.0), sing_2901, sing_2887);
    sub_3{}(sing_2880, sing_2894, sing_2901, sing_2887, M_PI, 8.851307);


}};
CUDAQ_REGISTER_OPERATION(
unitary_5, 
    1,    // Number of qubits
    0,    // Number of params
    (std::vector<std::complex<double>>{
           {0.37249965426779102, 0.2761955111526857},{-0.70653840173053095, -0.53456854945792465},
           {0.88464677928184499, -0.04858109813231351},{0.46325113801642193, -0.020936473843686403} 
    })
);

CUDAQ_REGISTER_OPERATION(
unitary_6, 
    2,    // Number of qubits
    0,    // Number of params
    (std::vector<std::complex<double>>{
           {-0.44230903096596369, 0.11579734239289241},{0.33762535417857981, -0.20771229952723491},{ -0.042452620536331691, -0.16385145396869394},{ 0.29183163090690756, -0.72111246883253088},
           {0.48840781896824798, -0.11214628803873801},{0.37514452326663511, -0.18776301872253481},{ -0.53956502283457541, 0.49047689657214716},{ 0.17939119946740192, -0.094939167518930945},
           {0.72724547977916032, 0.041671685485535556},{0.062017929629491306, -0.032298707622834183},{ 0.49314511446475101, -0.33756206664347332},{ 0.12947730131460186, -0.30097080979251523},
           {0.075194923932519198, -0.059648262798350199},{-0.46085686402974452, 0.67052060152629378},{ -0.14871273352292588, 0.24575749476973574},{ 0.042653410901113895, -0.494438413916791} 
    })
);

struct sub_7 { 
void operator()(cudaq::qubit& sing_3750,cudaq::qubit& sing_3757,cudaq::qubit& sing_3764,cudaq::qubit& sing_3771, double sing_3779,double sing_3786) __qpu__ {
    if (!mz(sing_3750)){
        z<cudaq::ctrl>(sing_3764, sing_3750);
        swap(sing_3771, sing_3764);
        sub_0{}(sing_3757, sing_3750, sing_3771, sing_3764, M_PI, 5.930395);
        if (!mz(sing_3771)){
            sub_3{}(sing_3771, sing_3764, sing_3750, sing_3757, (M_PI/4.0), sing_3786);
            sub_3{}(sing_3771, sing_3750, sing_3757, sing_3764, 4.214861, (M_PI/4.0));
            rz(4.859157, sing_3750);
            swap(sing_3757, sing_3750);
            r1(M_PI, sing_3757);
            y(sing_3757);
            s<cudaq::adj>(sing_3764);
            unitary_5(sing_3764);
            sub_3{}(sing_3750, sing_3757, sing_3771, sing_3764, 5.061573, sing_3779);
}       else {
            ry(sing_3779, sing_3757);
            x(sing_3764);
            swap(sing_3771, sing_3757);
            z<cudaq::ctrl>(sing_3750, sing_3757);
            ry(2.853135, sing_3757);
            unitary_5(sing_3750);
            x(sing_3750);
            x<cudaq::ctrl>(sing_3764, sing_3771);
            z<cudaq::ctrl>(sing_3764, sing_3750);
}
        u3(9.029793, 1.239891, sing_3779, sing_3750);
        swap(sing_3771, sing_3764);
        z<cudaq::ctrl>(sing_3757, sing_3771);
}   else {
        z(sing_3771);
        t<cudaq::adj>(sing_3750);
        sub_4{}(sing_3750, sing_3764, sing_3771, sing_3757, (M_PI/2.0), (M_PI/4.0));
        t<cudaq::adj>(sing_3771);
        unitary_5(sing_3750);
        sub_1{}(sing_3750, sing_3771, sing_3757, sing_3764, sing_3786, M_PI);
        sub_4{}(sing_3757, sing_3750, sing_3771, sing_3764, sing_3786, 3.160779);
        sub_3{}(sing_3771, sing_3757, sing_3764, sing_3750, sing_3786, 1.877651);
}
    sub_0{}(sing_3750, sing_3771, sing_3764, sing_3757, 8.803791, 0.519508);
    unitary_5(sing_3757);
    x<cudaq::ctrl>(sing_3771, sing_3764, sing_3757);
    y<cudaq::ctrl>(sing_3764, sing_3757);
    sub_0{}(sing_3750, sing_3764, sing_3771, sing_3757, sing_3786, 9.756884);
    unitary_6(sing_3757, sing_3750);
    t(sing_3757);


}};
CUDAQ_REGISTER_OPERATION(
unitary_8, 
    2,    // Number of qubits
    0,    // Number of params
    (std::vector<std::complex<double>>{
           {0.16509420512515155, -0.58656060160295675},{0.055849571705114807, 0.30663808263624143},{ -0.23395458694956703, 0.59112329457890356},{ -0.35319615759179018, -0.051336651747385879},
           {-0.64972392238549392, 0.17182710162412415},{-0.39684536490997346, 0.49427068749325287},{ -0.37595495091504383, -0.022658509106684491},{ -0.021055452413994275, 0.065158655856685635},
           {0.072238362313204421, -0.021607857110099205},{-0.098682518293513599, -0.31397139842046395},{ -0.37945312744895687, 0.32347757172594072},{ 0.48165109280532581, 0.63670106842810803},
           {-0.24933133628265447, 0.33041404355392506},{-0.12486955143155634, -0.61412984957122252},{ -0.15525645062633825, 0.42574169151890173},{ -0.35597633962835995, -0.32223106589385608} 
    })
);

struct sub_9 { 
void operator()(cudaq::qubit& sing_4601,cudaq::qubit& sing_4608,cudaq::qubit& sing_4615,cudaq::qubit& sing_4622, double sing_4630,double sing_4637) __qpu__ {
    rz<cudaq::ctrl>(M_PI, sing_4622, sing_4608);
    if (!mz(sing_4615)){
        if (!mz(sing_4601)){
            z(sing_4615);
            unitary_5(sing_4622);
            rx<cudaq::ctrl>(3.310598, sing_4608, sing_4601);
            t<cudaq::adj>(sing_4608);
            rz<cudaq::ctrl>(M_PI, sing_4601, sing_4615);
            sub_2{}(sing_4601, sing_4622, sing_4615, sing_4608, sing_4630, sing_4630);
            sub_4{}(sing_4615, sing_4601, sing_4608, sing_4622, 3.803664, (M_PI/2.0));
            sub_3{}(sing_4622, sing_4615, sing_4601, sing_4608, 7.010479, sing_4637);
}       else {
            unitary_5(sing_4608);
            x(sing_4608);
            unitary_8(sing_4601, sing_4615);
            y(sing_4615);
            sub_7{}(sing_4622, sing_4601, sing_4615, sing_4608, sing_4630, 8.721992);
            sub_0{}(sing_4615, sing_4608, sing_4601, sing_4622, 8.749058, M_PI);
            y(sing_4608);
}
        sub_1{}(sing_4601, sing_4622, sing_4615, sing_4608, 8.587988, M_PI);
        rz(1.640732, sing_4601);
        x<cudaq::ctrl>(sing_4622, sing_4601, sing_4615);
        sub_0{}(sing_4601, sing_4608, sing_4622, sing_4615, 5.116209, (M_PI/4.0));
        t<cudaq::adj>(sing_4622);
        sub_4{}(sing_4622, sing_4608, sing_4601, sing_4615, sing_4637, (M_PI/4.0));
        x(sing_4622);
        ry(5.969634, sing_4615);
}   else {
        ry<cudaq::ctrl>(8.098601, sing_4615, sing_4601);
        sub_4{}(sing_4615, sing_4622, sing_4608, sing_4601, (M_PI/4.0), sing_4637);
        ry<cudaq::ctrl>(M_PI, sing_4615, sing_4622);
        unitary_8(sing_4608, sing_4601);
        x(sing_4615);
        sub_1{}(sing_4601, sing_4622, sing_4615, sing_4608, sing_4630, (M_PI/2.0));
        rz(sing_4637, sing_4615);
        ry<cudaq::ctrl>(sing_4630, sing_4615, sing_4608);
        z<cudaq::ctrl>(sing_4608, sing_4622);
        unitary_5(sing_4608);
}
    x<cudaq::ctrl>(sing_4601, sing_4615, sing_4608);
    sub_3{}(sing_4622, sing_4601, sing_4608, sing_4615, (M_PI/4.0), sing_4637);
    sub_1{}(sing_4622, sing_4601, sing_4608, sing_4615, (M_PI/4.0), (M_PI/2.0));
    y<cudaq::ctrl>(sing_4608, sing_4615);
    swap(sing_4601, sing_4622);


}};

struct main_circuit { 
uint64_t operator()() __qpu__ {
    cudaq::qarray<2>reg_5447;
    cudaq::qubit sing_5459;
    cudaq::qubit sing_5468;
    cudaq::qubit sing_5477;

    double reg_5488[2];

for(int i = 0; i < 2; i++){
reg_5488[i] = 1.658954;
}

    double reg_5512[1];

for(int i = 0; i < 1; i++){
reg_5512[i] = 6.463584;
}

    double reg_5534[2];

for(int i = 0; i < 2; i++){
reg_5534[i] = M_PI;
}


    if (mz(sing_5477)){
        if (!mz(reg_5447[1])){
            // sub_9{}(reg_5447[0], sing_5459, reg_5447[1], sing_5468, (M_PI/4.0), reg_5534[0]);
            // sub_7{}(sing_5468, reg_5447[0], sing_5459, reg_5447[1], 5.516948, M_PI);
            z<cudaq::ctrl>(sing_5477, sing_5468);
            h(reg_5447[0]);
            // sub_9{}(reg_5447[1], sing_5459, reg_5447[0], sing_5468, (M_PI/4.0), (M_PI/4.0));
            unitary_8(reg_5447[0], reg_5447[1]);
            r1((M_PI/2.0), reg_5447[0]);
            unitary_8(sing_5477, sing_5468);
}
        t<cudaq::adj>(sing_5468);
        swap(sing_5468, reg_5447[1]);
        x<cudaq::ctrl>(reg_5447[1], sing_5459, sing_5468);
        // sub_2{}(reg_5447[0], sing_5477, sing_5468, sing_5459, reg_5488[1], (M_PI/2.0));
        unitary_6(reg_5447[1], sing_5459);
}   else {
        // sub_7{}(reg_5447[0], sing_5459, sing_5468, sing_5477, M_PI, 7.013969);
        s<cudaq::adj>(sing_5459);
        x<cudaq::ctrl>(sing_5459, sing_5468, reg_5447[0]);
        ry<cudaq::ctrl>(reg_5534[1], sing_5477, reg_5447[0]);
        sub_1{}(sing_5468, sing_5477, reg_5447[1], sing_5459, 7.216637, reg_5534[1]);
        unitary_5(reg_5447[0]);
        sub_9{}(sing_5477, reg_5447[1], sing_5459, reg_5447[0], M_PI, (M_PI/2.0));
}
    ry<cudaq::ctrl>(reg_5488[1], sing_5468, reg_5447[1]);
    // sub_4{}(reg_5447[1], sing_5459, sing_5468, reg_5447[0], reg_5512[0], (M_PI/2.0));
    h<cudaq::ctrl>(sing_5477, reg_5447[0]);
    // sub_9{}(sing_5468, reg_5447[0], sing_5477, sing_5459, reg_5512[0], M_PI);
    x<cudaq::ctrl>(sing_5477, sing_5468, reg_5447[1]);
    rz<cudaq::ctrl>(reg_5534[1], sing_5468, reg_5447[1]);
    ry<cudaq::ctrl>((M_PI/4.0), sing_5459, sing_5468);
    unitary_5(sing_5477);

    uint64_t packed_result = 0; int bit_offset = 0;

    for (auto b : mz(reg_5447)){
   if (b) packed_result |= (1ULL << bit_offset);
   bit_offset++;
}
    if (mz(sing_5459)){ packed_result |= (1ULL << bit_offset); }
   bit_offset += 1;
    if (mz(sing_5468)){ packed_result |= (1ULL << bit_offset); }
   bit_offset += 1;
    if (mz(sing_5477)){ packed_result |= (1ULL << bit_offset); }
   bit_offset += 1;
    return packed_result;
}};

int main(int argc, char* argv[]) {
    auto kernel = main_circuit{};

    int num_shots = std::stoi(argv[1]);
    try {
            auto results = cudaq::run(num_shots, kernel);
        std::map<std::string, std::size_t> counts;
        for (const uint64_t &res : results) {
           counts[std::to_string(res)]++;
        }
        printf("{");
        bool first = true;
        for (const auto &[val, count] : counts){
             if (!first) std::cout << ", ";
             printf("\"%s\" : %zu\n", val.c_str(), count);
             first = false;
        }
        printf("}");
    } catch (const std::exception& e) {
        std::cerr << "[CUDA-Q Exception] " << e.what() << std::endl;
        return 1;
    }
    return 0;
}"""

ct = cudaqTesting(source_code, 4373)
ct.opt_ks_test()
