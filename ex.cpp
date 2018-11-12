#include "ex.hpp"

#include <cmath>
#include <enulib/action.hpp>
#include <enulib/asset.hpp>
#include "enu.token.hpp"

using namespace enumivo;
using namespace std;

void ex::receivedenu(const currency::transfer &transfer) {
  if (transfer.to != _self) {
    return;
  }

  // get ENU balance
  double enu_balance = enumivo::token(N(enu.token)).
	   get_balance(N(enu.ftp.mm), enumivo::symbol_type(ENU_SYMBOL).name()).amount;
  
  enu_balance = enu_balance/10000;

  double received = transfer.quantity.amount;
  received = received/10000;

  // get FTP balance
  double ftp_balance = enumivo::token(N(ftp.coin)).
	   get_balance(N(enu.ftp.mm), enumivo::symbol_type(FTP_SYMBOL).name()).amount;

  ftp_balance = ftp_balance/10000;

  //deduct fee
  received = received * 0.997;
  
  double product = ftp_balance * enu_balance;

  double buy = ftp_balance - (product / (received + enu_balance));

  auto to = transfer.from;

  auto quantity = asset(10000*buy, FTP_SYMBOL);

  action(permission_level{N(enu.ftp.mm), N(active)}, N(ftp.coin), N(transfer),
         std::make_tuple(N(enu.ftp.mm), to, quantity,
                         std::string("Buy FTP with ENU")))
      .send();

  action(permission_level{_self, N(active)}, N(enu.token), N(transfer),
         std::make_tuple(_self, N(enu.ftp.mm), transfer.quantity,
                         std::string("Buy FTP with ENU")))
      .send();
}

void ex::receivedftp(const currency::transfer &transfer) {
  if (transfer.to != _self) {
    return;
  }

  // get FTP balance
  double ftp_balance = enumivo::token(N(ftp.coin)).
	   get_balance(N(enu.ftp.mm), enumivo::symbol_type(FTP_SYMBOL).name()).amount;
  
  ftp_balance = ftp_balance/10000;

  double received = transfer.quantity.amount;
  received = received/10000;

  // get ENU balance
  double enu_balance = enumivo::token(N(enu.token)).
	   get_balance(N(enu.ftp.mm), enumivo::symbol_type(ENU_SYMBOL).name()).amount;

  enu_balance = enu_balance/10000;

  //deduct fee
  received = received * 0.997;

  double product = enu_balance * ftp_balance;

  double sell = enu_balance - (product / (received + ftp_balance));

  auto to = transfer.from;

  auto quantity = asset(10000*sell, ENU_SYMBOL);

  action(permission_level{N(enu.ftp.mm), N(active)}, N(enu.token), N(transfer),
         std::make_tuple(N(enu.ftp.mm), to, quantity,
                         std::string("Sell FTP for ENU")))
      .send();

  action(permission_level{_self, N(active)}, N(ftp.coin), N(transfer),
         std::make_tuple(_self, N(enu.ftp.mm), transfer.quantity,
                         std::string("Sell FTP for ENU")))
      .send();
}

void ex::apply(account_name contract, action_name act) {

  if (contract == N(enu.token) && act == N(transfer)) {
    auto transfer = unpack_action_data<currency::transfer>();

    enumivo_assert(transfer.quantity.symbol == ENU_SYMBOL,
                 "Must send ENU");
    receivedenu(transfer);
    return;
  }

  if (contract == N(ftp.coin) && act == N(transfer)) {
    auto transfer = unpack_action_data<currency::transfer>();

    enumivo_assert(transfer.quantity.symbol == FTP_SYMBOL,
                 "Must send FTP");
    receivedftp(transfer);
    return;
  }

  if (act == N(transfer)) {
    auto transfer = unpack_action_data<currency::transfer>();
    enumivo_assert(false, "Must send FTP or ENU");
    return;
  }

  if (contract != _self) return;

}

extern "C" {
[[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
  ex enuftp(receiver);
  enuftp.apply(code, action);
  enumivo_exit(0);
}
}
