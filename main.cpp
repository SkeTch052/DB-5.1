#include <iostream>
#include <pqxx/pqxx>
#include <Windows.h>
#include <string>
#pragma execution_character_set(utf-8)

class DBInteraction {
private:
	std::string connectionString_;
public:

	DBInteraction(const std::string& connectionString) : connectionString_(connectionString) {}

	//ћетод, создающий структуру Ѕƒ (таблицы).
	void createTables() {
		pqxx::connection c(connectionString_);
		pqxx::work tx(c);

		tx.exec("CREATE TABLE IF NOT EXISTS Clients ("
			    "id SERIAL PRIMARY KEY, "
		    	"first_name TEXT, "
			    "last_name TEXT, "
			    "email TEXT)");
		tx.exec("CREATE TABLE IF NOT EXISTS PhoneNumbers ("
			    "id SERIAL PRIMARY KEY, "
			    "client_id INTEGER REFERENCES Clients(id), "
			    "phone_number TEXT)");
		tx.commit();
		std::cout << "Tables have been created." << std::endl;
	}

	//ћетод, позвол€ющий добавить нового клиента.
	void addClient() {
		pqxx::connection c(connectionString_);
		pqxx::work tx1(c);

		std::string first_name;
		std::cout << "Enter first_name: ";
		getline(std::cin, first_name);

		std::string last_name;
		std::cout << "Enter last_name: ";
		getline(std::cin, last_name);

		std::string email;
		std::cout << "Enter email: ";
		getline(std::cin, email);

		tx1.exec("INSERT INTO Clients(first_name, last_name, email) "
		 	     "VALUES('" + first_name + "', '" + last_name + "', '" + email + "');");

		tx1.commit();

		pqxx::work tx2(c);
		std::cout << "New client has been added. ";
		std::string clientid = tx2.query_value<std::string>("SELECT id FROM Clients "
		                                                    "WHERE first_name = '" + first_name + "' "
			                                                "AND last_name = '" + last_name + "' "
			                                                "AND email = '" + email + "';");
		std::cout << "Clients ID is: " << clientid << "." << std::endl;
	}

	//ћетод, позвол€ющий добавить телефон дл€ существующего клиента.
	void addPhone() {
		pqxx::connection c(connectionString_);
		pqxx::work tx(c);

		std::string id;
		std::cout << "Enter client ID: ";
		getline(std::cin, id);

		std::string phone;
		std::cout << "Enter phone number: ";
		getline(std::cin, phone);

		tx.exec("INSERT INTO PhoneNumbers (client_id, phone_number) "
			    "VALUES('" + id + "', '" + phone + "');");

		tx.commit();
		std::cout << "Phone number has been added." << std::endl;
	}

	//ћетод, позвол€ющий изменить данные о клиенте.
	void updateClient() {
		pqxx::connection c(connectionString_);
		pqxx::work tx(c);

		std::string id;
		std::cout << "Enter client ID to change: ";
		getline(std::cin, id);

		std::string first_name;
		std::cout << "Enter first_name: ";
		getline(std::cin, first_name);

		std::string last_name;
		std::cout << "Enter last_name: ";
		getline(std::cin, last_name);

		std::string email;
		std::cout << "Enter email: ";
		getline(std::cin, email);

		tx.exec("UPDATE Clients "
			    "SET first_name = '" + first_name + "', last_name = '" + last_name + "', email = '" + email + "' "
			    "WHERE id = '" + id +"' ");

		tx.commit();
		std::cout << "Client N." << id << " has been updated." << std::endl;
	}

	//ћетод, позвол€ющий удалить телефон у существующего клиента.
	void deletePhone() {
		pqxx::connection c(connectionString_);
		pqxx::work tx(c);

		std::string id;
		std::cout << "Enter client ID to delete phone number: ";
		getline(std::cin, id);

		pqxx::result res = tx.exec("SELECT * FROM PhoneNumbers "
			                       "WHERE client_id = " + id + ";");

		if (res.empty()) {
			std::cout << "No phone numbers." << std::endl;
			return;
		}

		if (res.size() > 1) {
			std::string phone;
			std::cout << "Enter phone number to delete: ";
			getline(std::cin, phone);

			tx.exec("DELETE FROM PhoneNumbers "
				    "WHERE client_id = " + id + " AND phone_number = '" + phone + "';");

			std::cout << "Phone number " << phone << " has been deleted." << std::endl;
		}
		else {tx.exec("DELETE FROM PhoneNumbers "
			          "WHERE client_id = " + id + ";");
			  std::cout << "Phone number has been deleted." << std::endl;
		}
		tx.commit();
	}

	//ћетод, позвол€ющий удалить существующего клиента.
	void deleteClient() {
		pqxx::connection c(connectionString_);
		pqxx::work tx(c);

		std::string id;
		std::cout << "Enter client ID to delete: ";
		getline(std::cin, id);

		tx.exec("DELETE FROM phonenumbers WHERE id = " + id + "; "
			    "DELETE FROM Clients WHERE id = " + id + ";");
		tx.commit();
		std::cout << "Client N." << id << " has been deleted." << std::endl;
	}

	//ћетод, позвол€ющий найти клиента по его данным Ч имени, фамилии, email или телефону.
	void findClient() {
		pqxx::connection c(connectionString_);
		pqxx::work tx(c);

		std::string searchValue;
		std::cout << "Enter details to search (first name, last name, emai or phone_number): ";
		getline(std::cin, searchValue);

		std::string sqlCommand =
			"SELECT Clients.id, Clients.first_name, Clients.last_name, Clients.email "
			"FROM Clients "
			"WHERE Clients.first_name ILIKE '%" + searchValue + "%' "
			"OR Clients.last_name ILIKE '%" + searchValue + "%' "
			"OR Clients.email ILIKE '%" + searchValue + "%' "
			"GROUP BY Clients.id;";

		pqxx::result r = tx.exec(sqlCommand);
		
		if (r.empty()) {
			sqlCommand =
				"SELECT Clients.id, Clients.first_name, Clients.last_name, Clients.email, STRING_AGG(PhoneNumbers.phone_number, ', ') AS phone_numbers "
				"FROM Clients "
				"LEFT JOIN PhoneNumbers ON Clients.id = PhoneNumbers.client_id "
				"WHERE PhoneNumbers.phone_number = '" + searchValue + "' "
				"GROUP BY Clients.id, PhoneNumbers.phone_number; ";

			r = tx.exec(sqlCommand);

			if (r.empty() || r.at(0).at(0).is_null()) {
				std::cout << "Client not found." << std::endl;
				return;
			}
			else {
				for (auto [id, first_name, last_name, email, phone] : tx.query<std::string, std::string, std::string, std::string, std::string>(sqlCommand))
				{
					std::cout << "ID: " << id << std::endl;
					std::cout << "First name: " << first_name << std::endl;
					std::cout << "Last name: " << last_name << std::endl;
					std::cout << "Email: " << email << std::endl;

					std::string phones = "SELECT STRING_AGG(PhoneNumbers.phone_number, ', ') AS pn "
						                 "FROM PhoneNumbers "
						                 "WHERE client_id = " + id + ";";
					pqxx::result r2 = tx.exec(phones);
					if (r2.at(0).at(0).is_null()) {
						std::cout << "Phones: -" << std::endl;
					}
					else {
						for (const auto& row : r2) {
							std::cout << "Phones: " << row["pn"].as<std::string>() << std::endl;
						}
					}
					std::cout << "-----------------------------" << std::endl;
				}
				tx.commit();
				return;
			}
		}
		for (auto [id, first_name, last_name, email] : tx.query<std::string, std::string, std::string, std::string>(sqlCommand))
		{
			std::cout << "ID: " << id << std::endl;
			std::cout << "First name: " << first_name << std::endl;
			std::cout << "Last name: " << last_name << std::endl;
			std::cout << "Email: " << email << std::endl;

			std::string phones = "SELECT STRING_AGG(PhoneNumbers.phone_number, ', ') AS pn "
					             "FROM PhoneNumbers "
					             "WHERE client_id = " + id + ";";
			pqxx::result r3 = tx.exec(phones);
			if (r3.at(0).at(0).is_null()) {
				std::cout << "Phones: -" << std::endl;
			}
			else {
 				for (const auto& row : r3) {
					std::cout << "Phones: " << row["pn"].as<std::string>() << std::endl;
				}
			}
			std::cout << "-----------------------------" << std::endl;
		}
		tx.commit();
	}
};

int main() {
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	setvbuf(stdout, nullptr, _IOFBF, 1000);

	try {
		DBInteraction MyDBI(
			"host=localhost "
			"port=5432 "
			"dbname=Homework_5_DB "
			"user=postgres "
			"password=mypass123");

		MyDBI.createTables();
		MyDBI.addClient();
		MyDBI.addPhone();
		MyDBI.updateClient();
		MyDBI.deletePhone();
	    MyDBI.deleteClient();
		MyDBI.findClient();

		std::cout << "Goodbye!" << std::endl;
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
		}
	return 0;
}