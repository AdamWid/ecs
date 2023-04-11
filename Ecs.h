#pragma once
#include "Assert.h"
#include <cstdint>
#include <type_traits>
#include <algorithm>
#include "Entity.h"
#include "Reference.h"
#include "Heap.hpp"
#include <tuple>
#include "Exclude.h"
#include "EntityIterator.h"
#include "UpdateContext.h"
#include <vector>

namespace ecs
{
    template <typename...>
    struct TList;

    class Family
    {
    public:
        template <typename>
        static Entity type() noexcept
        {
            static const Entity value = identifier();
            return value;
        }
    private:
        static Entity identifier() noexcept
        {
            static Entity value = 0;
            return value++;
        }
    };

    template <typename T>
    class SparseSet
    {
    public:
        using IdType = Entity;

        SparseSet() : dense(nullptr), sparse(nullptr), size(0), capacity(0), sparse_capacity(0)
        {}

        ~SparseSet()
        {
            //delete[] mirror;
            delete[] dense;
            delete[] sparse;
        }

        T& operator[](IdType index)
        {
            return mirror[index];
        }
        
        void Clear()
        {
            delete[] dense;
            delete[] sparse;
            dense = nullptr;
            sparse = nullptr;
            size = 0;
            capacity = 0;
            sparse_capacity = 0;
        }

        template <typename... Args>
        T& Emplace(IdType id, Args&&... args)
        {
            ECS_ASSERT(!Contains(id));
            Grow(id);
            dense[size] = id;
            sparse[id] = size++;
            mirror.emplace_back(std::forward<Args>(args)...);
            
            return mirror[size-1];
        }

        void Remove(IdType id)
        {
            ECS_ASSERT(Contains(id) && "Removing nonexisting element");
            IdType denseIndex = sparse[id];

            --size;
            std::swap(mirror.back(), mirror[denseIndex]);
            std::swap(dense[size], dense[denseIndex]);
            sparse[dense[denseIndex]] = denseIndex;

            auto tmp = std::move(mirror.back()); // Have to do this since it seems back is changed after destructor is called on object
            mirror.pop_back();
        }

        inline bool Contains(IdType id) const
        {
            return id < sparse_capacity && sparse[id] < size && dense[sparse[id]] == id;
        }

        IdType& DenseFront()
        {
            return *dense;
        }

        T& Front()
        {
            ECS_ASSERT(size && "Set is empty");
            return mirror.front();
        }

        T& Get(IdType id)
        {
            return mirror[sparse[id]];
        }

        const T& Get(IdType id) const
        {
            return mirror[sparse[id]];
        }

        size_t Size() const
        {
            return size;
        }

    private:
        inline void Grow(IdType id)
        {
            if (size >= capacity)
            {
                capacity = capacity * 2 + 1;
                IdType* tmp = new IdType[capacity];
                memcpy(tmp, dense, size * sizeof(IdType));
                delete[] dense;
                dense = tmp;
            }
            if (id >= sparse_capacity)
            {
                IdType tmpcap = sparse_capacity;
                sparse_capacity = id * 2 + 1;
                IdType* tmp = new IdType[sparse_capacity];
                memcpy(tmp, sparse, tmpcap * sizeof(IdType));
                delete[] sparse;
                sparse = tmp;
            }
        }

        IdType size;
        IdType capacity;

        IdType sparse_capacity;

        std::vector<T> mirror;
        IdType* dense;
        IdType* sparse;
    };

    class IContainer
    {
    public:
        virtual ~IContainer() = default;
        virtual bool Contains(Entity aEntity) const = 0;
        virtual size_t Size() const = 0;
        virtual Entity& DenseFront() = 0;
        virtual Entity& DenseBack() = 0;
        virtual void Destroy(Entity aEntity) = 0;

        virtual void Update(mys::UpdateContext& anUpdateContext) = 0;
        virtual void Start() = 0;

        virtual void OnCollisionEnter(Entity aOwner, Entity aEntering) = 0;
        virtual void OnCollisionExit(Entity aOwner, Entity aEntering) = 0;

        virtual void OnTriggerEnter(Entity aOwner, Entity aEntering) = 0;
        virtual void OnTriggerExit(Entity aOwner, Entity aEntering) = 0;
    };

    namespace detail
    {

        // From https://stackoverflow.com/questions/87372/check-if-a-class-has-a-member-function-of-a-given-signature

#define DEFINE_HAS_METHOD(Name)                                                                         \
        template <typename, typename T>                                                                 \
        struct Has##Name                                                                                \
        {                                                                                               \
            static_assert(std::integral_constant<T, false>::value, "how did we get here");              \
        };                                                                                              \
                                                                                                        \
        template <typename C, typename Ret, typename... Args>                                           \
        struct Has##Name##<C, Ret(Args...)>                                                             \
        {                                                                                               \
        private:                                                                                        \
                                                                                                        \
            template <typename T>                                                                       \
            static constexpr auto check(T*)                                                             \
                -> typename std::is_same<decltype(std::declval<T>().##Name##(std::declval<Args>()...)), \
                Ret>::type;                                                                             \
                                                                                                        \
            template <typename>                                                                         \
            static constexpr std::false_type check(...);                                                \
            typedef decltype(check<C>(0)) type;                                                         \
        public:                                                                                         \
            static constexpr bool value = type::value;                                                  \
        };                                                                                              \

        DEFINE_HAS_METHOD(Update);
        DEFINE_HAS_METHOD(Create);
        DEFINE_HAS_METHOD(Start);

        DEFINE_HAS_METHOD(OnCollisionEnter);
        DEFINE_HAS_METHOD(OnCollisionExit);

        DEFINE_HAS_METHOD(OnTriggerEnter);
        DEFINE_HAS_METHOD(OnTriggerExit);
    }

    template <typename T>
    class Container : public IContainer
    {
    public:

        template <typename... Args>
        T& Emplace(Entity aEntity, Args&&... args)
        {
            return myTypes.Emplace(aEntity, args...);
        }

        const T& Get(Entity aEntity) const
        {
            return myTypes.Get(aEntity);
        }

        T& Get(Entity aEntity)
        {
            return myTypes.Get(aEntity);
        }

        ecs::Entity GetEntityOf(T& someType)
        {
            ECS_ASSERT(Contains(someType));
            return *(&myTypes.DenseFront() + (&someType - &myTypes.Front()));
        }

        template <typename Func>
        void Sort(Func&& aComparator)
        {

        }

        size_t Size() const override
        {
            return myTypes.Size();
        }

        bool Contains(T& someType)
        {
            return (&someType - &myTypes.Front()) < myTypes.Size();
        }

        bool Contains(Entity aEntity) const override
        {
            //return aEntity < sparse_capacity && sparse[aEntity] < size && dense[sparse[aEntity]] == aEntity;
            return myTypes.Contains(aEntity);
        }

        Entity& DenseFront() override
        {
            return myTypes.DenseFront();
        }

        Entity& DenseBack() override
        {
            return *(&myTypes.DenseFront() + myTypes.Size() - 1);
        }

        void Destroy(Entity aEntity) override
        {
            if (myTypes.Contains(aEntity))
                myTypes.Remove(aEntity);
        }

        void Update(mys::UpdateContext& anUpdateContext) override
        {
            if constexpr (detail::HasUpdate<T, void(mys::UpdateContext&)>::value)
            {
                for (Entity i = 0; i < myTypes.Size(); ++i)
                    myTypes[i].Update(anUpdateContext);
            }
        }

        void Start() override
        {
            if constexpr (detail::HasStart<T, void(void)>::value)
            {
                for (Entity i = 0; i < myTypes.Size(); ++i)
                    myTypes[i].Start();
            }
        }

        void OnCollisionEnter(Entity aOwner, Entity aEntering) override
        {
            if constexpr (detail::HasOnCollisionEnter<T, void(Entity)>::value)
            {
                myTypes.Get(aOwner).OnCollisionEnter(aEntering);
                /*for (Entity i = 0; i < myTypes.Size(); ++i)
                    myTypes[i].OnCollisionEnter(aEntity);*/
            }
        }

        void OnCollisionExit(Entity aOwner, Entity aExiting) override
        {
            if constexpr (detail::HasOnCollisionExit<T, void(Entity)>::value)
            {
                myTypes.Get(aOwner).OnCollisionExit(aExiting);
            }
        }

        void OnTriggerEnter(Entity aOwner, Entity aEntering) override
        {
            if constexpr (detail::HasOnTriggerEnter<T, void(Entity)>::value)
            {
                myTypes.Get(aOwner).OnTriggerEnter(aEntering);
            }
        }

        void OnTriggerExit(Entity aOwner, Entity aExiting) override
        {
            if constexpr (detail::HasOnTriggerExit<T, void(Entity)>::value)
            {
                myTypes.Get(aOwner).OnTriggerExit(aExiting);
            }
        }

    private:
        //std::vector<std::shared_ptr<std::array<T, 1000>> mirror;
        //std::vector<Entity> dense;
        //std::vector<Entity> sparse;
        SparseSet<T> myTypes;
    };

    template <typename It>
    class IIterator
    {
    public:
        IIterator(It aBegin, It aEnd) : myBegin(aBegin), myEnd(aEnd)
        {}

        It begin()
        {
            return myBegin;
        }

        It end()
        {
            return myEnd;
        }

    private:
        It myBegin;
        It myEnd;
    };

    template <typename, typename>
    class TypeViewIterator;

    template <typename... Types, typename... Excludes>
    class TypeViewIterator<TList<Types...>, TList<Excludes...>>
    {
    public:
        using ValueType = Entity;
        using PointerType = Entity*;
        using ReferenceType = Entity&;
    public:

        TypeViewIterator(const TypeViewIterator&) = default;
        TypeViewIterator(TypeViewIterator&&) = default;

        TypeViewIterator(Entity* aEntity, std::tuple<Container<Types>*...> someContainer, std::tuple<Container<Excludes>*...> someExcludes, Entity* aEnd) :
            it(aEntity), arr(std::move(someContainer)), excludes(std::move(someExcludes)), end(aEnd) {
            while (it != end && !Valid(*it) && ++it != end);
        }

        PointerType operator->()
        {
            return it;
        }

        ReferenceType operator*()
        {
            return *it;
        }

        bool operator!=(const TypeViewIterator& aRhs)
        {
            return it != aRhs.it;
        }

        bool operator==(const TypeViewIterator& aRhs)
        {
            return it == aRhs.it;
        }

        inline TypeViewIterator& operator++()
        {
            while (++it != end && !Valid(*it));

            return *this;
        }

        inline TypeViewIterator& operator++(int)
        {
            TypeViewIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        inline std::tuple<Container<Types>*...>& Tuple()
        {
            return arr;
        }

        bool Valid(Entity aEntity)
        {
            bool types = std::apply([aEntity](auto* ...container) -> bool
            {
                size_t contains = 0;

                contains = (container->Contains(aEntity) + ...);

                return contains == sizeof...(Types);
            }, arr);


            if constexpr (sizeof...(Excludes) > 0)
            {
                bool excluded = std::apply([aEntity](auto* ...container) -> bool
                {
                    size_t contains = 0;

                    contains = (container->Contains(aEntity) + ...);

                    return contains == 0;
                }, excludes);

                return types && excluded;
            }
            else
                return types;
        }
    private:
        std::tuple<Container<Types>*...> arr;
        std::tuple<Container<Excludes>*...> excludes;
        Entity* it;
        Entity* end;
    };

    template <typename, typename>
    class TypeViewEachIterator;

    template <typename... Types, typename... Excludes>
    class TypeViewEachIterator<TList<Types...>, TList<Excludes...>>
    {
    private:
        using IteratorType = TypeViewIterator<TList<Types...>, TList<Excludes...>>;
    public:
        TypeViewEachIterator(IteratorType&& aIterator) : it(aIterator)
        {}

        std::tuple<Entity, Types&...> operator*()
        {
            return std::apply(
                [entity = *it](auto* ...container) 
                {
                    return std::tuple_cat(std::tuple<Entity>(entity), std::forward_as_tuple(container->Get(entity))...);
                },
                it.Tuple()
            );
        }

        bool operator!=(const TypeViewEachIterator& aRhs)
        {
            return it != aRhs.it;
        }

        bool operator==(const TypeViewEachIterator& aRhs)
        {
            return it == aRhs.it;
        }

        inline TypeViewEachIterator& operator++()
        {
            while (++it != it && it.Valid(*it));
            return *this;
        }

        inline TypeViewEachIterator& operator++(int)
        {
            TypeViewEachIterator tmp = *this;
            ++(*this);
            return tmp;
        }

    private:
        IteratorType it;
    };

    template <typename T1, typename T2>
    class TypeView;

    template <typename... Types, typename... Excludes>
    class TypeView<TList<Types...>, TList<Excludes...>>
    {
    public:
        using Iterator = TypeViewIterator<TList<Types...>, TList<Excludes...>>;
        using EachIterator = TypeViewEachIterator<TList<Types...>, TList<Excludes...>>;
        using EachIteratorWrapper = IIterator<TypeViewEachIterator<TList<Types...>, TList<Excludes...>>>;
    public:

        TypeView(const std::tuple<Container<Types>*...>& someTypes, const std::tuple<Container<Excludes>*...>& someExcludes) :
            excludes(someExcludes),
            types(someTypes),
            smallest{
                std::apply([](auto* ...container) -> IContainer*
                {
                    return (std::min)({((IContainer*)container)...},
                    [](auto* lhs, auto* rhs)
                    {
                        return lhs->Size() < rhs->Size();
                    }
                );
            }, someTypes) }
        {}
        
            

        Iterator begin()
        {
            return Iterator(&smallest->DenseFront(), types, excludes, (&smallest->DenseBack()) + 1);
        }

        Iterator end()
        {
            return Iterator((&smallest->DenseBack()) + 1, types, excludes, (&smallest->DenseBack()) + 1);
        }

        EachIteratorWrapper Each()
        {
            return EachIteratorWrapper(EachIterator(begin()), EachIterator(end()));
        }

    private:

        IContainer* smallest;
        std::tuple<Container<Types>*...> types;
        std::tuple<Container<Excludes>*...> excludes;
    };

    class Registry
    {
    public:
        friend class EntityIterator;

        ~Registry()
        {
            myIsCurrentlySelfDestructing = true;
            for (Entity i = 0; i < myContainers.Size(); ++i)
                delete myContainers[i];
        }

        void Clear()
        {
            myIsCurrentlySelfDestructing = true;
            for (Entity i = 0; i < myContainers.Size(); ++i)
                delete myContainers[i];

            myNext = 0;
            myEntityQueue.Clear();
            myEntityDestroyList.clear();
            myEntityDestroyListAfterTime.clear();
            myContainers.Clear();
            myPollingstationPtr = nullptr;
        }

        Entity Create()
        {
            return (myEntityQueue.Size()) ? myEntityQueue.Dequeue() : myNext++;
        }

        void Destroy(Entity aEntity)
        {
            ECS_ASSERT_VALID_ENTITY(!myEntityQueue.Contains(aEntity) && "Destroying invalid entity");
            ECS_ASSERT(aEntity != nullentity);
            for (Entity i = 0; i < myContainers.Size(); ++i)
                myContainers[i]->Destroy(aEntity);

            //ECS_ASSERT(!myEntityQueue.Contains(aEntity));
            myEntityQueue.Enqueue(aEntity);
            //ECS_ASSERT(myEntityQueue.Contains(aEntity));
        }

        void Destroy(Entity aEntity, const float aTime)
        {
            ECS_ASSERT_VALID_ENTITY(Valid(aEntity));
            ECS_ASSERT(aEntity != nullentity);
            myEntityDestroyListAfterTime.push_back({ aTime, aEntity });
        }

        void LateDestroy(Entity aEntity)
        {
            ECS_ASSERT_VALID_ENTITY(Valid(aEntity));
            ECS_ASSERT(aEntity != nullentity && "Cant't destroy null entity");
            myEntityDestroyList.push_back(aEntity);
        }

        bool Valid(Entity aEntity)
        {
            return aEntity < myNext && aEntity != ecs::nullentity
                && !myEntityQueue.Contains(aEntity)
                && (std::find(myEntityDestroyList.begin(), myEntityDestroyList.end(), aEntity) == myEntityDestroyList.end())
                && (std::find_if(myEntityDestroyListAfterTime.begin(), myEntityDestroyListAfterTime.end(), [entity = aEntity](std::pair<float, Entity>& aPair) {return aPair.second == entity; }) == myEntityDestroyListAfterTime.end());
        }

        template <typename T, typename... Args>
        T& Emplace(Entity aEntity, Args&&... args)
        {
            return GetContainer<T>()->Emplace(aEntity, args...);
        }

        template <typename T>
        const T& Get(Entity aEntity) const
        {
            ECS_ASSERT(aEntity != nullentity);

            const Entity id = Family::type<T>();

            ECS_ASSERT((myContainers.Size() && myContainers.Contains(id)) && "No such components exist");

            Container<T>* c = (Container<T>*)myContainers.Get(id);
            ECS_ASSERT(c->Contains(aEntity) && "Entity has no such component");
            return c->Get(aEntity);
        }

        template <typename T>
        T& Get(Entity aEntity)
        {
            ECS_ASSERT(aEntity != nullentity);

            const Entity id = Family::type<T>();

            ECS_ASSERT((myContainers.Size() && myContainers.Contains(id)) && "No such components exist");

            Container<T>* c = (Container<T>*)myContainers.Get(id);
            ECS_ASSERT(c->Contains(aEntity) && "Entity has no such component");
            return c->Get(aEntity);
        }

        template <typename T>
        T* TryGet(Entity aEntity)
        {
            if (Contains<T>(aEntity))
                return &Get<T>(aEntity);
            return nullptr;
        }
        template <typename T>
        const T* TryGet(Entity aEntity) const
        {
            if (Contains<T>(aEntity))
                return &Get<T>(aEntity);
            return nullptr;
        }

        template <typename T>
        void Remove(Entity aEntity)
        {
            ECS_ASSERT(aEntity != nullentity);

            const Entity id = Family::type<T>();

            ECS_ASSERT((myContainers.Size() && myContainers.Contains(id)) && "No such components exist");

            Container<T>* c = (Container<T>*)myContainers.Get(id);
            ECS_ASSERT(c->Contains(aEntity) && "Entity has no such component");
            c->Destroy(aEntity);
        }

        template <typename T>
        bool Contains(Entity aEntity)
        {
            const Entity id = Family::type<T>();
            return myContainers.Contains(id) && myContainers.Get(id)->Contains(aEntity);
        }

        inline bool IsCurrentlyDestructingDONOTUSE()
        {
            return myIsCurrentlySelfDestructing;
        }

        void Update(mys::UpdateContext& anUpdateContext)
        {
            for (Entity i = 0; i < myContainers.Size(); ++i)
                myContainers[i]->Update(anUpdateContext);

            for (Entity i = 0; i < myEntityDestroyList.size(); ++i)
                Destroy(myEntityDestroyList[i]);
            myEntityDestroyList.clear();

            for (Entity i = 0; i < myEntityDestroyListAfterTime.size(); ++i)
            {
                std::pair<float, Entity>& e = myEntityDestroyListAfterTime[i];
                e.first -= anUpdateContext.timeDelta;
                if (e.first <= 0)
                {
                    Destroy(e.second);
                    std::swap(e, myEntityDestroyListAfterTime.back());
                    myEntityDestroyListAfterTime.pop_back();
                    --i;
                    continue;
                }
            }
        }

        void OnCollisionEnter(Entity aOwner, Entity aEntering)
        {
            for (Entity i = 0; i < myContainers.Size(); ++i)
                if (myContainers[i]->Contains(aOwner))
                    myContainers[i]->OnCollisionEnter(aOwner, aEntering);
        }

        void OnCollisionExit(Entity aOwner, Entity aExiting)
        {
            for (Entity i = 0; i < myContainers.Size(); ++i)
                if (myContainers[i]->Contains(aOwner))
                    myContainers[i]->OnCollisionExit(aOwner, aExiting);
        }

        void OnTriggerEnter(Entity aOwner, Entity aEntering)
        {
            for (Entity i = 0; i < myContainers.Size(); ++i)
                if (myContainers[i]->Contains(aOwner))
                    myContainers[i]->OnTriggerEnter(aOwner, aEntering);
        }

        void OnTriggerExit(Entity aOwner, Entity aExiting)
        {
            for (Entity i = 0; i < myContainers.Size(); ++i)
                if (myContainers[i]->Contains(aOwner))
                    myContainers[i]->OnTriggerExit(aOwner, aExiting);
        }

        void Start()
        {
            for (Entity i = 0; i < myContainers.Size(); ++i)
                myContainers[i]->Start();
        }

        template <typename T, typename Func>
        void Sort(Func&& aComparator)
        {
            GetContainer<T>()->Sort(aComparator);
        }

        template <typename T>
        Reference<T> CreateReference(Entity aEntity)
        {
            return Reference<T>(aEntity, GetContainer<T>());
        }

        template <typename T>
        ecs::Entity GetEntityOf(T& someType)
        {
            const Entity id = Family::type<T>();
            ECS_ASSERT((myContainers.Size() && myContainers.Contains(id)) && "No such components exist");

            Container<T>* c = (Container<T>*)myContainers.Get(id);
            ECS_ASSERT(c->Contains(someType) && "Component is not part of registry");
            return c->GetEntityOf(someType);
        }

        template <typename F>
        void Inspect(Entity aEntity, F&& aFunctor)
        {}

        template <typename Type1, typename... Types, typename F>
        void Inspect(Entity aEntity, F&& aFunctor)
        {
            //assert(aEntity != nullentity);

            Container<Type1>* c = GetContainer<Type1>();
            if (c->Contains(aEntity))
                aFunctor(c->Get(aEntity));
            Inspect<Types...>(aEntity, aFunctor);
        }

        template <typename T1, typename... Types>
        TypeView<TList<T1, Types...>, TList<>> View()
        {
            auto c = GetContainer<T1>();
            return { std::make_tuple(c, GetContainer<Types>()...), std::make_tuple() };
        }

        template <typename T1, typename... Types, typename... Excludes>
        TypeView<TList<T1, Types...>, TList<Excludes...>> View(ecs::Exclude<Excludes...>)
        {
            auto c = GetContainer<T1>();
            return { std::make_tuple(c, GetContainer<Types>()...), std::make_tuple(GetContainer<Excludes>()...) };
        }

        inline EntityIteratorWrapper Entities()
        {
            EntityIterator a(*this, 0, myNext);
            EntityIterator b(*this, myNext, myNext);

            return { a,b };
        }

        inline void SetPollingStation(mys::PollingStation* aPollingStation) { myPollingstationPtr = aPollingStation; }
    private:

        template <typename T>
        Container<T>* GetContainer()
        {
            const Entity id = Family::type<T>();
            if (myContainers.Size() && myContainers.Contains(id))
                return (Container<T>*)myContainers.Get(id);

            Container<T>* c = new Container<T>();
            myContainers.Emplace(id, c);
            return c;
        }

        bool myIsCurrentlySelfDestructing = false; // REMOVE THIS
        Entity myNext{};
        mys::Heap<Entity, mys::Less<Entity>> myEntityQueue;
        std::vector<Entity> myEntityDestroyList;
        std::vector<std::pair<float, Entity>> myEntityDestroyListAfterTime;
        SparseSet<IContainer*> myContainers;
        mys::PollingStation* myPollingstationPtr;
    };
}