#include <halley.hpp>
using namespace Halley;

class IConfigDatabaseType {
public:
    IConfigDatabaseType(String key)
	    : key(std::move(key))
    {}

    virtual ~IConfigDatabaseType() = default;

    const String& getKey()
    {
        return key;
    }

    virtual void loadConfigs(const ConfigNode& nodes) = 0;

private:
    String key;
};

template <typename T>
class ConfigDatabaseType : public IConfigDatabaseType {
public:
    ConfigDatabaseType(String key)
		: IConfigDatabaseType(std::move(key))
    {}

    const T& get(std::string_view id) const
    {
        return entries.at(id);
    }

    const T* tryGet(std::string_view id) const
    {
        const auto iter = entries.find(id);
        if (iter != entries.end()) {
            return &iter->second;
        }
        return nullptr;
    }

    bool contains(std::string_view id) const
    {
        return entries.contains(id);
    }

    Vector<String> getKeys() const
    {
        Vector<String> result;
        result.resize(entries.size());
        for (const auto& e: entries) {
            result.push_back(e.first);
        }
        return result;
    }

    const HashMap<String, T>& getEntries() const
    {
        return entries;
    }

    HashMap<String, T>& getEntries()
    {
        return entries;
    }

    void loadConfigs(const ConfigNode& nodes)
    {
	    for (const auto& n: nodes.asSequence()) {
            loadConfig(n);
	    }
    }

    void loadConfig(const ConfigNode& node)
    {
    	entries[node["id"].asString()] = T(node);
    }

private:
    HashMap<String, T> entries;
};

class ConfigDatabase {
public:
    void load(Resources& resources, const String& prefix);

    template <typename T>
    void init(String key)
    {
        dbs[std::type_index(typeid(T))] = std::make_unique<ConfigDatabaseType<T>>(std::move(key));
    }

    template <typename T>
    const T& get(std::string_view id) const
    {
        return of<T>().get(id);
    }

    template <typename T>
    const T* tryGet(std::string_view id) const
    {
        return of<T>().tryGet(id);
    }

    template <typename T>
    bool contains(std::string_view id) const
    {
        return of<T>().contains(id);
    }

    template <typename T>
    Vector<String> getKeys() const
    {
        return of<T>().getKeys();
    }

    template <typename T>
    const HashMap<String, T>& getEntries() const
    {
        return of<T>().getEntries();
    }

private:
    HashMap<std::type_index, std::unique_ptr<IConfigDatabaseType>> dbs;

    template <typename T>
    ConfigDatabaseType<T>& of() const
    {
        return static_cast<ConfigDatabaseType<T>&>(*dbs.at(std::type_index(typeid(T))));
    }
};
