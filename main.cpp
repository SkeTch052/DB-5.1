#include <iostream>
#include <pqxx/pqxx>
#include <Windows.h>
#include <string>
#pragma execution_character_set(utf-8)

class ClientManager {
private:
	std::string connectionString_;
public:

	ClientManager(const std::string& connectionString) : connectionString_(connectionString) {}

	//ћетод, создающий структуру Ѕƒ (таблицы).
	void createTables() {
		pqxx::connection c(connectionString_);
		pqxx::work tx(c);

		tx.exec("CREATE TABLE IF NOT EXISTS Clients ("
			    "id SERIAL PRIMARY KEY, "
		    	"first_name TEXT NOT NULL, "
			    "last_name TEXT NOT NULL, "
			    "email TEXT NOT NULL UNIQUE);");
		tx.exec("CREATE TABLE IF NOT EXISTS PhoneNumbers ("
			    "id SERIAL PRIMARY KEY, "
			    "client_id INTEGER REFERENCES Clients(id), "
			    "phone_number TEXT);");
		tx.commit();
	}

	//ћетод, провер€ющий есть ли клиент в Ѕƒ.
	bool isClientExists(int clientId) {
		pqxx::connection c(connectionString_);
		pqxx::work txn(c);

		pqxx::result r = txn.exec_params("SELECT id FROM Clients WHERE id = $1;", clientId);
		if (r.empty()) {
			std::cout << "Client not found.\n" << std::endl;
			return false;
		}
		else {
			return true;
		}
	}

	//ћетод, позвол€ющий добавить нового клиента.
	int addClient(const std::string& firstName, const std::string& lastName, const std::string& email) {
		pqxx::connection c(connectionString_);
		pqxx::work tx1(c);

		std::string sqlCommand = "INSERT INTO Clients(first_name, last_name, email) "
			                     "VALUES ($1, $2, $3);";

		tx1.exec_params(sqlCommand, firstName, lastName, email);
		tx1.commit();

		pqxx::work tx2(c);

		sqlCommand = "SELECT id FROM Clients "
			         "WHERE first_name = $1 AND last_name = $2 AND email = $3;";

		pqxx::result r = tx2.exec_params(sqlCommand, firstName, lastName, email);
		std::cout << "New client has been added! ID is: ";
		int clientid = r[0][0].as<int>();
		return clientid;
	}

	//ћетод, позвол€ющий добавить телефон дл€ существующего клиента.
	void addPhone(int clientId, const std::string& phoneNumber) {
		pqxx::connection c(connectionString_);
		pqxx::work tx(c);

		if (isClientExists(clientId) == false) {
			return;
		}

		std::string sqlCommand = "INSERT INTO PhoneNumbers (client_id, phone_number) "
		                         "VALUES($1, $2);";
		tx.exec_params(sqlCommand, clientId, phoneNumber);
		tx.commit();
		std::cout << "You added a new phone number." << std::endl;
	}

	//ћетод, позвол€ющий изменить данные о клиенте.
	void updateClient(int clientId, const std::string& firstName, const std::string& lastName, const std::string& email) {
		pqxx::connection c(connectionString_);
		pqxx::work tx(c);

		if (isClientExists(clientId) == false) {
			return;
		}

		std::string sqlCommand = "UPDATE Clients "
			                     "SET first_name = $2, last_name = $3, email = $4 "
			                     "WHERE id = $1;";
		tx.exec_params(sqlCommand, clientId, firstName, lastName, email);
		tx.commit();
		std::cout << "The client's data has been changed.\n" << std::endl;
	}

	//ћетод, позвол€ющий удалить телефон у существующего клиента.
	void removePhone(int clientId) {
		pqxx::connection c(connectionString_);
		pqxx::work tx(c);

		if (isClientExists(clientId) == false) {
			return;
		}

		pqxx::result r = tx.exec_params("SELECT * FROM PhoneNumbers WHERE client_id = $1;", clientId);
		
		if (r.empty()) {
			std::cout << "No phone numbers.\n" << std::endl;
			return;
		}
		else if (r.size() > 1) {
			pqxx::result r2 = tx.exec_params("SELECT id, phone_number FROM phonenumbers WHERE client_id = $1 GROUP BY id;", clientId);

			std::map<int, std::string> phoneNumbers;

			std::cout << "Your phone numbers: " << std::endl;

			for (auto row : r2) {
				std::cout << "ID: " << row[0] << "\t" << "Phone: " << row[1] << std::endl;
				phoneNumbers.insert({ row[0].as<int>(), row[1].as<std::string>() });
			}

			int enter_id;
			while (true) {
				std::cout << "Enter phone ID to remove: ";
				std::cin >> enter_id;
				if (phoneNumbers.find(enter_id) == phoneNumbers.end()) {
					std::cout << "Invalid ID!" << std::endl;
				}
				else {
					tx.exec_params("DELETE FROM PhoneNumbers WHERE id = $1;", enter_id);
					tx.commit();
					std::cout << "Phone number has been removed.\n" << std::endl;
					return;
				}
			}
		}
		else {
			tx.exec_params("DELETE FROM PhoneNumbers WHERE client_id = $1;", clientId);
			tx.commit();
			std::cout << "Phone number has been removed.\n" << std::endl;
		}
	}
	
	//ћетод, позвол€ющий удалить существующего клиента.
	void removeClient(int clientId) {
		pqxx::connection c(connectionString_);
		pqxx::work tx(c);

		if (isClientExists(clientId) == false) {
			return;
		}

		tx.exec_params("DELETE FROM phonenumbers WHERE client_id = $1;", clientId);
		tx.exec_params("DELETE FROM Clients WHERE id = $1;", clientId);
		tx.commit();
		std::cout << "Client with ID:" << clientId << " has been removed.\n" << std::endl;
	}

	//ћетод, позвол€ющий найти клиента по его данным Ч имени, фамилии, email или телефону.
	void findClient(const std::string& searchValue) {
		pqxx::connection c(connectionString_);
		pqxx::work tx(c);

		std::string likeSearchValue = "%" + searchValue + "%";

		std::string sqlCommand =
			"SELECT Clients.id, Clients.first_name, Clients.last_name, Clients.email, STRING_AGG(PhoneNumbers.phone_number, ', ') AS all_phone_numbers "
			"FROM Clients "
			"LEFT JOIN PhoneNumbers ON Clients.id = PhoneNumbers.client_id "
			"WHERE Clients.first_name ILIKE $1 "
			"OR Clients.last_name ILIKE $1 "
			"OR Clients.email ILIKE $1 "
			"OR PhoneNumbers.phone_number = $2 "
			"GROUP BY Clients.id;";
    
		pqxx::result r = tx.exec_params(sqlCommand, likeSearchValue, searchValue);

        if (r.empty()) {
			std::cout << "Client not found." << std::endl;
        }
		for (const auto& row : r)
		{
			std::cout << "ID: " << row[0].as<std::string>() << std::endl;
			std::cout << "First name: " << row[1].as<std::string>() << std::endl;
			std::cout << "Last name: " << row[2].as<std::string>() << std::endl;
			std::cout << "Email: " << row[3].as<std::string>() << std::endl;
			if (!row[4].is_null()) {
				std::string phones = "SELECT STRING_AGG(PhoneNumbers.phone_number, ', ') AS all_phone_numbers "
					                 "FROM PhoneNumbers "
					                 "WHERE client_id = " + row[0].as<std::string>() + ";";
				pqxx::result r2 = tx.exec(phones);
				for (const auto& row : r2) {
					std::cout << "Phones: " << row["all_phone_numbers"].as<std::string>() << std::endl;
				}
			}
			else {
				std::cout << "Phones: -" << std::endl;
			}
			std::cout << "-----------------------------" << std::endl;
		}
	}
};

int main() {
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	setvbuf(stdout, nullptr, _IOFBF, 1000);

	try {
		ClientManager CM(
			"host=localhost "
			"port=5432 "
			"dbname=Homework_5_DB "
			"user=postgres "
			"password=mypass123");

		CM.createTables();

        std::cout << CM.addClient("Igor", "Manenkov", "igormanenkov@mail.ru") << std::endl;
		std::cout << CM.addClient("Andrey", "Smirnov", "andeykutuzov@mail.ru") << std::endl;
		std::cout << CM.addClient("Igor", "Ivanov", "igorivanov@mail.ru") << std::endl;
		std::cout << CM.addClient("Petr", "Petrov", "pertpetrov@mail.ru") << std::endl;

		std::cout << std::endl;
        CM.addPhone(1, "111111"); 
		CM.addPhone(1, "222222");
		CM.addPhone(1, "333333");
		CM.addPhone(2, "444444");

		std::cout << std::endl;
		CM.updateClient(2, "Andrey", "Kutuzov", "andeykutuzov@mail.ru");

		CM.removePhone(1);

		CM.removeClient(4);

		CM.findClient("Igor");
		//CM.findClient("444444");
		//CM.findClient("Veronika");

		std::cout << "Goodbye!" << std::endl;
	}
	catch (const std::exception& e) {
		std::cout << e.what() << std::endl;
		}
	return 0;
}